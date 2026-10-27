#pragma once
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

// Android key codes for volume buttons
#define KEY_VOLUMEDOWN  114
#define KEY_VOLUMEUP    115

struct InputState {
    std::atomic<bool>     vol_down{false};   // single press pending
    std::atomic<bool>     vol_up{false};
    std::atomic<bool>     double_press{false}; // double-press (either button) detected
    std::atomic<bool>     running{true};
    std::thread           reader;
    // double-press tracking (written only from reader thread)
    int64_t               last_vol_ms{0};
    static constexpr int  DOUBLE_MS = 400;
};

// Find all /dev/input/eventX devices
static std::vector<std::string> find_input_devices() {
    std::vector<std::string> devs;
    DIR* d = opendir("/dev/input");
    if (!d) return devs;
    struct dirent* ent;
    while ((ent = readdir(d))) {
        if (strncmp(ent->d_name, "event", 5) == 0) {
            devs.push_back(std::string("/dev/input/") + ent->d_name);
        }
    }
    closedir(d);
    return devs;
}

// Background thread: open all input devices, poll for volume key events
static void input_reader_thread(InputState* st) {
    auto devs = find_input_devices();
    std::vector<int> fds;
    for (auto& p : devs) {
        int fd = open(p.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd >= 0) fds.push_back(fd);
    }
    fprintf(stderr, "[INPUT] opened %zu input devices\n", fds.size());

    auto now_ms = []() -> int64_t {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    };

    while (st->running.load()) {
        for (int fd : fds) {
            struct input_event ev;
            while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_KEY && ev.value == 1) {  // key down
                    bool is_vol = (ev.code == KEY_VOLUMEDOWN || ev.code == KEY_VOLUMEUP);
                    if (is_vol) {
                        int64_t now = now_ms();
                        int64_t diff = now - st->last_vol_ms;
                        if (diff > 0 && diff < st->DOUBLE_MS) {
                            // second press within window — double press
                            st->double_press.store(true);
                            st->last_vol_ms = 0;  // reset so triple doesn't trigger again
                        } else {
                            st->last_vol_ms = now;
                        }
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    for (int fd : fds) close(fd);
}

static void input_init(InputState& st) {
    st.reader = std::thread(input_reader_thread, &st);
}

static void input_destroy(InputState& st) {
    st.running.store(false);
    if (st.reader.joinable()) st.reader.join();
}

// Returns true if a double-press (either vol button) happened this frame
static bool input_consume_double_press(InputState& st) {
    return st.double_press.exchange(false);
}
