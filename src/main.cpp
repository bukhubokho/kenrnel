#include "draw/draw.h"
#include "ui/menu.hpp"
#include "ui/bar.hpp"
#include "ui/theme/theme.hpp"
#include "other/diag.hpp"
#include "protect/oxorany.hpp"
#include "driver/driver.h"
#include "ff/ff_pid.hpp"
#include "ff/ff_read.hpp"
#include "ff/esp_draw.hpp"

#include <cstdio>
#include <thread>
#include <chrono>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

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

    native_window_screen_x = max_size;
    native_window_screen_y = max_size;
    g_sw = (float)max_size;
    g_sh = (float)min_size;

    diag::log("INFO", "display %dx%d orientation=%d overlay=%dx%d",
              displayInfo.width, displayInfo.height, displayInfo.orientation,
              native_window_screen_x, native_window_screen_y);

    int drv_fd = rt_open();
    if (drv_fd < 0) {
        diag::log("ERROR", "RTkernel driver not found, run 5.10.sh first");
        return 1;
    }
    diag::log("INFO", "RTkernel driver OK fd=%d", drv_fd);

    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true)) {
        diag::log("ERROR", "initGUI_draw failed");
        rt_close(drv_fd);
        return -1;
    }
    diag::log("INFO", "GUI initialized");

    touch::init(max_size, min_size, (uint8_t)displayInfo.orientation);

    static ff::PlayerInfo s_players[ff::MAX_PLAYERS];
    static int            s_player_count = 0;
    static uint32_t       s_last_pid     = 0;

    while (true) {
        drawBegin();

        // ── FF PID + kernel read ──────────────────────────────────────────────
        uint32_t pid = (uint32_t)ff::get_pid();
        if (pid != s_last_pid) {
            s_last_pid     = pid;
            s_player_count = 0;
            if (pid) ff::init(drv_fd, pid);
        }
        if (pid) {
            s_player_count = ff::update(drv_fd, pid, s_players);
            if (ff::g_base)
                esp::update_camera(drv_fd, pid, ff::g_base);
        }

        // ── ESP — drawn behind menu ───────────────────────────────────────────
        esp::draw(ImGui::GetBackgroundDrawList(),
                  s_players, s_player_count,
                  Menu::cfg, g_sw, g_sh);

        // ── Menu / bar ────────────────────────────────────────────────────────
        ui::bar::set_game_alpha(1.0f);
        ui::menu::render();

        if (ui::menu::should_exit()) break;

        drawEnd();
        usleep(1500);
    }

    rt_close(drv_fd);
    shutdown();
    return 0;
}
