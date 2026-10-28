#pragma once
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

#define KEY_VOLUMEDOWN  114
#define KEY_VOLUMEUP    115

// ── Wolsaw-style touch input ──────────────────────────────────────────────────
// One thread per /dev/input/event* that has ABS_MT_POSITION_X.
// Writes directly to ImGuiIO::MousePos / MouseDown per rotation.

#define TOUCH_MODE 2   // ABS_MT_TRACKING_ID-based (Wolsaw default)

struct InputState {
    std::atomic<bool> double_press{false};
    std::atomic<bool> running{true};
    std::vector<std::thread> readers;

    // double-press tracking
    std::mutex   vol_mtx;
    int64_t      last_vol_ms{0};
    static constexpr int DOUBLE_MS = 400;

    // current screen dims + rotation — updated each frame from overlay
    std::atomic<int> screen_w{1080};
    std::atomic<int> screen_h{2400};
    std::atomic<int> rotation{0};    // 0=portrait,1=90CW,2=180,3=90CCW
};

// Per-device structure (mirrors touch::input from Wolsaw)
struct DevInput {
    int fd{-1};
    bool canBeUsed{false};
    struct input_absinfo absX{}, absY{};
    float absXMultiplier{0.f}, absYMultiplier{0.f};
#if TOUCH_MODE == 2
    bool trackingIDPresent{false};
#endif

    bool open_dev(const char* path, int32_t sw, int32_t sh) {
        fd = ::open(path, O_RDWR);
        if (fd == -1) return false;
        canBeUsed = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX) != -1
                 && ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY) != -1
                 && absX.maximum > 0 && absY.maximum > 0;
        if (!canBeUsed) { close_dev(); return false; }
#if TOUCH_MODE == 2
        struct input_absinfo absT{};
        trackingIDPresent = ioctl(fd, EVIOCGABS(ABS_MT_TRACKING_ID), &absT) != -1
                         && absT.minimum <= 0 && absT.maximum > 0;
#endif
        calc_multipliers(sw, sh);
        return true;
    }

    void close_dev() {
        if (fd != -1) { ::close(fd); fd = -1; }
    }

    void calc_multipliers(int32_t sw, int32_t sh) {
        float mx = fmaxf((float)absX.maximum, (float)absY.maximum);
        float mn = fminf((float)absX.maximum, (float)absY.maximum);
        absXMultiplier = (float)sw / (sw > sh ? mx : mn);
        absYMultiplier = (float)sh / (sw > sh ? mn : mx);
    }
};

// ── Imported from imgui at pump time ──────────────────────────────────────────
#include "../imgui/imgui.h"

static void touch_thread(DevInput dev, InputState* st) {
    ::sleep(1);

    struct input_event events[64]{};
    static bool isDown = false;
    static float x = 0.f, y = 0.f;

    auto now_ms = []() -> int64_t {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    };

    while (dev.fd > 0 && st->running.load()) {
        ssize_t n = read(dev.fd, events, sizeof(events));
        if (n <= 0) { ::usleep(4000); continue; }
        int cnt = (int)(n / sizeof(struct input_event));

        for (int j = 0; j < cnt; ++j) {
            auto& ev = events[j];

            // Volume double-press
            if (ev.type == EV_KEY && ev.value == 1 &&
                (ev.code == KEY_VOLUMEDOWN || ev.code == KEY_VOLUMEUP)) {
                std::lock_guard<std::mutex> lk(st->vol_mtx);
                int64_t now  = now_ms();
                int64_t diff = now - st->last_vol_ms;
                if (diff > 0 && diff < st->DOUBLE_MS) {
                    st->double_press.store(true);
                    st->last_vol_ms = 0;
                } else {
                    st->last_vol_ms = now;
                }
            }

            if (!dev.canBeUsed) continue;

            if (ev.type == EV_ABS) {
#if TOUCH_MODE == 2
                if (dev.trackingIDPresent && ev.code == ABS_MT_TRACKING_ID) {
                    isDown = ev.value != -1; continue;
                }
#elif TOUCH_MODE == 0
                if (ev.code == ABS_MT_TRACKING_ID) {
                    isDown = ev.value != -1; continue;
                }
#endif
                if (ev.code == ABS_MT_POSITION_X) { x = (float)ev.value; continue; }
                if (ev.code == ABS_MT_POSITION_Y) { y = (float)ev.value; continue; }
            }

            if (ev.code == SYN_REPORT) {
                ImGuiIO* io = &ImGui::GetIO();
#if TOUCH_MODE == 2
                io->MouseDown[0] = dev.trackingIDPresent ? isDown : cnt > 2;
#elif TOUCH_MODE == 0
                io->MouseDown[0] = isDown;
#else
                io->MouseDown[0] = cnt > 2;
#endif
                if (io->MouseDown[0]) {
                    int rot = st->rotation.load();
                    int sw  = st->screen_w.load();
                    int sh  = st->screen_h.load();
                    // Recalculate multipliers for current screen size
                    float mx = fmaxf((float)dev.absX.maximum, (float)dev.absY.maximum);
                    float mn = fminf((float)dev.absX.maximum, (float)dev.absY.maximum);
                    float xm = (float)sw / (sw > sh ? mx : mn);
                    float ym = (float)sh / (sw > sh ? mn : mx);

                    float px, py;
                    switch (rot) {
                    case 1:   // 90° CW (home left)
                        px = y;
                        py = dev.absX.maximum - x;
                        break;
                    case 2:   // 180°
                        px = dev.absX.maximum - x;
                        py = dev.absY.maximum - y;
                        break;
                    case 3:   // 90° CCW (home right)
                        px = dev.absY.maximum - y;
                        py = x;
                        break;
                    default:  // portrait
                        px = x;
                        py = y;
                        break;
                    }
                    io->MousePos.x = px * xm;
                    io->MousePos.y = py * ym;
                }
            }
        }
    }
    dev.close_dev();
}

static void input_init(InputState& st) {
    int sw = st.screen_w.load();
    int sh = st.screen_h.load();

    DIR* d = opendir("/dev/input");
    if (!d) return;
    struct dirent* ent;
    while ((ent = readdir(d))) {
        if (strncmp(ent->d_name, "event", 5) != 0) continue;
        char path[128];
        snprintf(path, sizeof(path), "/dev/input/%s", ent->d_name);
        DevInput dev{};
        if (dev.open_dev(path, sw, sh)) {
            st.readers.emplace_back(touch_thread, dev, &st);
            fprintf(stderr, "[INPUT] %s ok (touch=%d)\n", path, dev.canBeUsed);
        }
    }
    closedir(d);
    fprintf(stderr, "[INPUT] %zu reader threads started\n", st.readers.size());
}

static void input_destroy(InputState& st) {
    st.running.store(false);
    for (auto& t : st.readers) if (t.joinable()) t.join();
    st.readers.clear();
}

static bool input_consume_double_press(InputState& st) {
    return st.double_press.exchange(false);
}

// Touch is now written directly to ImGuiIO by touch_thread.
// This is kept for API compatibility — main loop still calls it.
static void input_pump_imgui(InputState& /*st*/) {}
