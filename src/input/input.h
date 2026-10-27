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
    std::atomic<bool> vol_down{false};   // pressed this frame
    std::atomic<bool> vol_up{false};
    std::atomic<bool> running{true};
    std::thread       reader;
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

    while (st->running.load()) {
        for (int fd : fds) {
            struct input_event ev;
            while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_KEY && ev.value == 1) {  // key down
                    if (ev.code == KEY_VOLUMEDOWN) st->vol_down.store(true);
                    if (ev.code == KEY_VOLUMEUP)   st->vol_up.store(true);
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

// Call once per frame — returns true if button was pressed, clears the flag
static bool input_consume_voldown(InputState& st) {
    bool v = st.vol_down.exchange(false);
    return v;
}
static bool input_consume_volup(InputState& st) {
    bool v = st.vol_up.exchange(false);
    return v;
}
