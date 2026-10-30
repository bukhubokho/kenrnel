// External build entry point — uses /proc/<pid>/mem (pread64), no kernel driver.
// Compile target: ffext  (CMakeLists.txt with USE_PROC_MEM defined)
#include "draw/draw.h"
#include "ui/menu.hpp"
#include "ui/bar.hpp"
#include "ui/theme/theme.hpp"
#include "other/diag.hpp"
#include "protect/oxorany.hpp"
#include "mem/reader.hpp"
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
    const int instance_lock = ::open("/data/local/tmp/ffext.lock", O_CREAT | O_RDWR, 0600);
    if (instance_lock < 0 || flock(instance_lock, LOCK_EX | LOCK_NB) != 0) {
        diag::log("ERROR", "another instance already running");
        if (instance_lock >= 0) ::close(instance_lock);
        return 2;
    }

    diag::log("INFO", "ffext (external proc_mem) starting");
    screen_config();

    int max_size = (displayInfo.height > displayInfo.width ? displayInfo.height : displayInfo.width);
    int min_size = (displayInfo.height < displayInfo.width ? displayInfo.height : displayInfo.width);

    native_window_screen_x = max_size;
    native_window_screen_y = max_size;
    g_sw = (float)max_size;
    g_sh = (float)min_size;

    diag::log("INFO", "display %dx%d overlay=%dx%d",
              displayInfo.width, displayInfo.height,
              native_window_screen_x, native_window_screen_y);

    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true)) {
        diag::log("ERROR", "initGUI_draw failed");
        return -1;
    }
    diag::log("INFO", "GUI initialized");

    touch::init(max_size, min_size, (uint8_t)displayInfo.orientation);

    static ff::PlayerInfo s_players[ff::MAX_PLAYERS];
    static int            s_player_count = 0;
    static uint32_t       s_last_pid     = 0;
    static int            s_mem_fd       = -1;  // /proc/<pid>/mem fd

    while (true) {
        drawBegin();

        // ── Find FF pid and open /proc/<pid>/mem ──────────────────────────────
        uint32_t pid = (uint32_t)ff::get_pid();
        if (pid != s_last_pid) {
            // Close old fd when pid changes (process restarted)
            if (s_mem_fd >= 0) { mem::close(s_mem_fd); s_mem_fd = -1; }
            s_last_pid     = pid;
            s_player_count = 0;
            if (pid) {
                s_mem_fd = mem::open(pid);  // opens /proc/<pid>/mem
                if (s_mem_fd < 0) {
                    diag::log("WARN", "mem::open pid=%u failed — check root", pid);
                } else {
                    diag::log("INFO", "proc_mem fd=%d for pid=%u", s_mem_fd, pid);
                    ff::init(s_mem_fd, pid);
                }
            }
        }
        if (pid && s_mem_fd >= 0) {
            s_player_count = ff::update(s_mem_fd, pid, s_players);
            esp::update_camera(s_mem_fd, pid, ff::g_base, ff::g_dbg.match);
        }

        // ── ESP ───────────────────────────────────────────────────────────────
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

    if (s_mem_fd >= 0) mem::close(s_mem_fd);
    shutdown();
    return 0;
}
