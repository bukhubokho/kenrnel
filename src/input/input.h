#pragma once
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#define KEY_VOLUMEDOWN  114
#define KEY_VOLUMEUP    115

struct TouchEvent {
    float x, y;
    bool  down;   // true = finger down, false = finger up
    bool  valid;
};

struct InputState {
    std::atomic<bool>  double_press{false};
    std::atomic<bool>  running{true};
    std::thread        reader;

    // Touch state — written by reader, read by main thread each frame
    std::mutex         touch_mtx;
    std::vector<TouchEvent> touch_queue;

    // double-press tracking
    int64_t            last_vol_ms{0};
    static constexpr int DOUBLE_MS = 400;

    // portrait (natural) dims — set once at init, never changed
    int portrait_w{1080};
    int portrait_h{2400};

    // current screen dims — updated each frame from EGL surface size
    std::atomic<int>   screen_w{1080};
    std::atomic<int>   screen_h{2400};

    // rotation: 0=portrait, 1=90°CW, 2=180°, 3=90°CCW
    std::atomic<int>   rotation{0};
};

static std::vector<std::string> find_input_devices() {
    std::vector<std::string> devs;
    DIR* d = opendir("/dev/input");
    if (!d) return devs;
    struct dirent* ent;
    while ((ent = readdir(d)))
        if (strncmp(ent->d_name, "event", 5) == 0)
            devs.push_back(std::string("/dev/input/") + ent->d_name);
    closedir(d);
    return devs;
}

// Per-device touch tracking
struct DevTouch {
    int   fd;
    int   abs_x_min, abs_x_max;
    int   abs_y_min, abs_y_max;
    int   cur_x{0}, cur_y{0};
    bool  touching{false};
    bool  is_touch_dev{false};
};

static void input_reader_thread(InputState* st) {
    auto devs = find_input_devices();
    std::vector<DevTouch> touch_devs;

    auto now_ms = []() -> int64_t {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    };

    for (auto& p : devs) {
        int fd = open(p.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        DevTouch dt; dt.fd = fd;
        // Check if device has ABS_MT_POSITION_X — touchscreen
        struct input_absinfo ai{};
        if (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &ai) == 0) {
            dt.abs_x_min = ai.minimum; dt.abs_x_max = ai.maximum;
            ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &ai);
            dt.abs_y_min = ai.minimum; dt.abs_y_max = ai.maximum;
            dt.is_touch_dev = (dt.abs_x_max > dt.abs_x_min);
        }
        touch_devs.push_back(dt);
    }
    fprintf(stderr, "[INPUT] opened %zu devices\n", touch_devs.size());

    while (st->running.load()) {
        // rotation is updated by main loop from overlay_query_size — no polling needed here

        for (auto& dt : touch_devs) {
            struct input_event ev;
            while (read(dt.fd, &ev, sizeof(ev)) == sizeof(ev)) {
                // Volume double-press
                if (ev.type == EV_KEY && ev.value == 1 &&
                    (ev.code == KEY_VOLUMEDOWN || ev.code == KEY_VOLUMEUP)) {
                    int64_t now = now_ms();
                    int64_t diff = now - st->last_vol_ms;
                    if (diff > 0 && diff < st->DOUBLE_MS) {
                        st->double_press.store(true);
                        st->last_vol_ms = 0;
                    } else {
                        st->last_vol_ms = now;
                    }
                }

                // Touch events
                if (!dt.is_touch_dev) continue;
                if (ev.type == EV_ABS) {
                    if (ev.code == ABS_MT_POSITION_X || ev.code == ABS_X)
                        dt.cur_x = ev.value;
                    if (ev.code == ABS_MT_POSITION_Y || ev.code == ABS_Y)
                        dt.cur_y = ev.value;
                }
                if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
                    dt.touching = (ev.value == 1);
                }
                if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
                    if (dt.abs_x_max <= dt.abs_x_min) continue;
                    // Store normalized portrait coords (0..1) — pump remaps per rotation
                    float nx = (float)(dt.cur_x - dt.abs_x_min) / (dt.abs_x_max - dt.abs_x_min);
                    float ny = (float)(dt.cur_y - dt.abs_y_min) / (dt.abs_y_max - dt.abs_y_min);
                    TouchEvent te{};
                    te.x     = nx;  // normalized, not pixels
                    te.y     = ny;
                    te.down  = dt.touching;
                    te.valid = true;
                    std::lock_guard<std::mutex> lk(st->touch_mtx);
                    st->touch_queue.push_back(te);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }

    for (auto& dt : touch_devs) close(dt.fd);
}

static void input_init(InputState& st) {
    // portrait dims set by caller before init; copy as baseline for rotation fallback
    st.portrait_w = st.screen_w.load();
    st.portrait_h = st.screen_h.load();
    st.reader = std::thread(input_reader_thread, &st);
}

static void input_destroy(InputState& st) {
    st.running.store(false);
    if (st.reader.joinable()) st.reader.join();
}

static bool input_consume_double_press(InputState& st) {
    return st.double_press.exchange(false);
}

// Call each frame — drains touch queue into ImGui, remaps for current rotation
#include "../imgui/imgui.h"
static void input_pump_imgui(InputState& st) {
    std::vector<TouchEvent> events;
    {
        std::lock_guard<std::mutex> lk(st.touch_mtx);
        events.swap(st.touch_queue);
    }
    ImGuiIO& io = ImGui::GetIO();
    int sw  = st.screen_w.load();
    int sh  = st.screen_h.load();
    int rot = st.rotation.load();

    for (auto& te : events) {
        // te.x/y are normalized portrait coords [0..1]
        float nx = te.x, ny = te.y;
        float sx, sy;
        switch (rot) {
        case 1:  // 90° CW — phone rotated right, USB on right
            sx = ny * sw;
            sy = (1.0f - nx) * sh;
            break;
        case 3:  // 90° CCW — phone rotated left, USB on left
            sx = (1.0f - ny) * sw;
            sy = nx * sh;
            break;
        case 2:  // 180° — upside down
            sx = (1.0f - nx) * sw;
            sy = (1.0f - ny) * sh;
            break;
        default: // 0 — portrait, direct map
            sx = nx * sw;
            sy = ny * sh;
            break;
        }
        io.AddMousePosEvent(sx, sy);
        io.AddMouseButtonEvent(0, te.down);
    }
}
