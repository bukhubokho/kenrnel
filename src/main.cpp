#include "draw/draw.h"
#include "ui/menu.hpp"
#include "ui/bar.hpp"
#include "other/diag.hpp"
#include "protect/oxorany.hpp"
#include "driver/driver.h"

#include <cstdio>
#include <thread>
#include <chrono>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

static bool driver_loaded() {
    int fd = rt_open();
    if (fd >= 0) { close(fd); return true; }
    return false;
}

int main() {
    const int instance_lock = ::open("/data/local/tmp/kenrnel.lock", O_CREAT | O_RDWR, 0600);
    if (instance_lock < 0 || flock(instance_lock, LOCK_EX | LOCK_NB) != 0) {
        diag::log("ERROR", "another instance already running");
        if (instance_lock >= 0) ::close(instance_lock);
        return 2;
    }

    diag::log("INFO", "kenrnel FF 1.0 starting");
    screen_config();

    int max_size = (displayInfo.height > displayInfo.width ? displayInfo.height : displayInfo.width);
    int min_size = (displayInfo.height < displayInfo.width ? displayInfo.height : displayInfo.width);

    // Square overlay buffer — no rotation issues
    native_window_screen_x = max_size;
    native_window_screen_y = max_size;

    diag::log("INFO", "display %dx%d orientation=%d overlay=%dx%d",
              displayInfo.width, displayInfo.height, displayInfo.orientation,
              native_window_screen_x, native_window_screen_y);

    if (!driver_loaded()) {
        diag::log("ERROR", "RTkernel driver not found, run 5.10.sh first");
        return 1;
    }
    diag::log("INFO", "RTkernel driver OK");

    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true)) {
        diag::log("ERROR", "initGUI_draw failed");
        return -1;
    }
    diag::log("INFO", "GUI initialized");

    touch::init(max_size, min_size, (uint8_t)displayInfo.orientation);

    while (true) {
        drawBegin();

        bool is_landscape;
#if defined(__x86_64__)
        is_landscape = (displayInfo.orientation == 0 || displayInfo.orientation == 2);
#else
        is_landscape = (displayInfo.orientation == 1 || displayInfo.orientation == 3);
#endif

        if (is_landscape) {
            ui::bar::set_game_alpha(1.0f);
            if (ui::bar::g_open) {
                ui::menu::render();
            }
        }

        if (ui::menu::should_exit()) break;

        drawEnd();
        usleep(ui::bar::g_open ? 1500 : 4000);
    }

    shutdown();
    return 0;
}
