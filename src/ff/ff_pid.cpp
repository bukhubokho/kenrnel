// language: C++17, target: Android arm64
// Scans /proc/<pid>/cmdline for the Free Fire package name.
// Result is cached and re-validated every ~1 second.
#include "ff_pid.hpp"
#include <dirent.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>

namespace ff {

static int32_t s_pid      = 0;
static time_t  s_last_chk = 0;

static int32_t scan_proc() {
    DIR* d = opendir("/proc");
    if (!d) return 0;

    dirent* ent;
    while ((ent = readdir(d)) != nullptr) {
        if (ent->d_type != DT_DIR) continue;
        // only numeric entries are PIDs
        const char* nm = ent->d_name;
        if (nm[0] < '1' || nm[0] > '9') continue;

        char path[64];
        snprintf(path, sizeof(path), "/proc/%s/cmdline", nm);

        FILE* f = fopen(path, "r");
        if (!f) continue;

        char buf[256] = {};
        fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);

        if (strstr(buf, PACKAGE)) {
            closedir(d);
            return (int32_t)atoi(nm);
        }
    }
    closedir(d);
    return 0;
}

int32_t get_pid() {
    time_t now = time(nullptr);
    if (now - s_last_chk < 1) return s_pid;   // use cache

    s_last_chk = now;

    // validate cached pid still alive
    if (s_pid > 0) {
        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/cmdline", s_pid);
        FILE* f = fopen(path, "r");
        if (f) {
            char buf[256] = {};
            fread(buf, 1, sizeof(buf) - 1, f);
            fclose(f);
            if (strstr(buf, PACKAGE)) return s_pid;
        }
        s_pid = 0;
    }

    s_pid = scan_proc();
    return s_pid;
}

} // namespace ff
