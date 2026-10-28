// language: C++17, file: volume_key.cpp, target: Android/Linux
// Scans /dev/input/event* for volume keys; double-press within window_ms toggles menu.
#include "volume_key.hpp"
#include "../ui/bar.hpp"

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <cstring>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <cstdint>

namespace volume_key {

    static std::atomic<bool> g_running{false};
    static uint32_t g_window_ms = 500;

    static bool has_volume_key(int fd) {
        uint8_t bits[(KEY_MAX + 1) / 8] = {};
        if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bits)), bits) < 0)
            return false;
        auto test = [&](int key) -> bool {
            return (bits[key / 8] >> (key % 8)) & 1;
        };
        return test(KEY_VOLUMEUP) || test(KEY_VOLUMEDOWN);
    }

    static void listen_thread(int fd) {
        using clock = std::chrono::steady_clock;
        clock::time_point last_press{};
        bool first = false;

        input_event ev{};
        while (g_running.load()) {
            ssize_t n = read(fd, &ev, sizeof(ev));
            if (n != sizeof(ev)) continue;

            if (ev.type != EV_KEY) continue;
            if (ev.code != KEY_VOLUMEUP && ev.code != KEY_VOLUMEDOWN) continue;
            if (ev.value != 1) continue;  // press only, ignore repeat/release

            auto now = clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_press).count();

            if (first && ms <= (long long)g_window_ms) {
                // double-press detected
                ui::bar::g_open = !ui::bar::g_open;
                first = false;
            } else {
                first = true;
                last_press = now;
            }
        }
        ::close(fd);
    }

    void init(uint32_t window_ms) {
        g_window_ms = window_ms;
        g_running.store(true);

        DIR* dir = opendir("/dev/input/");
        if (!dir) return;
        dirent* ptr;
        while ((ptr = readdir(dir))) {
            if (!strstr(ptr->d_name, "event")) continue;
            char path[128];
            snprintf(path, sizeof(path), "/dev/input/%s", ptr->d_name);
            int fd = open(path, O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;
            if (has_volume_key(fd)) {
                // reopen blocking for clean reads in thread
                ::close(fd);
                fd = open(path, O_RDONLY);
                if (fd >= 0)
                    std::thread(listen_thread, fd).detach();
            } else {
                ::close(fd);
            }
        }
        closedir(dir);
    }

    void shutdown() {
        g_running.store(false);
    }

}
