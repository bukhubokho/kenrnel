// language: C++17, target: Android arm64
// Uses screencap to capture the full composited display (game + overlay).
// screencap goes through SurfaceFlinger so every visible layer is included.
#include "screenshot.hpp"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sys/stat.h>

namespace screenshot {

bool take(int /*width*/, int /*height*/) {
    const char* dir = "/sdcard/DCIM/Screenshots";
    mkdir(dir, 0755);

    time_t t = time(nullptr);
    struct tm tm{}; localtime_r(&t, &tm);
    char path[256];
    snprintf(path, sizeof(path),
        "%s/claudepro_%04d%02d%02d_%02d%02d%02d.png",
        dir,
        tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);

    // screencap -p captures full composited screen (all SurfaceFlinger layers)
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "screencap -p '%s'", path);
    int rc = system(cmd);
    if (rc != 0) return false;

    // Notify gallery
    snprintf(cmd, sizeof(cmd),
        "am broadcast -a android.intent.action.MEDIA_SCANNER_SCAN_FILE "
        "-d 'file://%s' >/dev/null 2>&1 &", path);
    system(cmd);

    return true;
}

} // namespace screenshot
