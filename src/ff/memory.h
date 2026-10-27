#pragma once
#include "offsets.h"
#include "../driver/driver.h"
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <vector>

// State shared across modules
struct FFState {
    int      drv_fd   = -1;
    uint32_t ff_pid   = 0;
    uint64_t il2cpp   = 0;   // libil2cpp.so base
    bool     ready    = false;
};

inline uint32_t find_ff_pid(const char* pkg = "com.dts.freefireth") {
    DIR* d = opendir("/proc");
    if (!d) return 0;
    struct dirent* ent;
    while ((ent = readdir(d))) {
        if (ent->d_type != DT_DIR) continue;
        char* end;
        long pid = strtol(ent->d_name, &end, 10);
        if (*end) continue;

        char path[64], cmdline[256]{};
        snprintf(path, sizeof(path), "/proc/%ld/cmdline", pid);
        int f = open(path, O_RDONLY);
        if (f < 0) continue;
        read(f, cmdline, sizeof(cmdline)-1);
        close(f);

        if (strncmp(cmdline, pkg, strlen(pkg)) == 0) {
            closedir(d);
            return (uint32_t)pid;
        }
    }
    closedir(d);
    return 0;
}

// Parse /proc/pid/maps to find libil2cpp.so base address
inline uint64_t find_il2cpp_base(uint32_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%u/maps", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "libil2cpp.so") && strstr(line, "r-xp")) {
            uint64_t base;
            sscanf(line, "%lx", &base);
            fclose(f);
            return base;
        }
    }
    fclose(f);
    return 0;
}

inline bool ff_init(FFState& st) {
    st.drv_fd = rt_open();
    if (st.drv_fd < 0) return false;

    st.ff_pid = find_ff_pid();
    if (!st.ff_pid) {
        // Try max variant
        st.ff_pid = find_ff_pid("com.dts.freefiremax");
    }
    if (!st.ff_pid) return false;

    // Let RTkernel store the target mm
    const char* pkg = "com.dts.freefireth";
    st.il2cpp = rt_set_pid(st.drv_fd, st.ff_pid, pkg);

    // If driver didn't return base, parse maps ourselves
    if (!st.il2cpp) {
        st.il2cpp = find_il2cpp_base(st.ff_pid);
    }

    st.ready = (st.il2cpp != 0);
    return st.ready;
}

struct Entity {
    uint64_t  ptr        = 0;
    FF::Vec3  pos        = {};
    FF::Vec3  head_pos   = {};
    float     hp         = 0;
    float     max_hp     = 100;
    int       team_id    = -1;
    bool      is_dead    = false;
    char      name[32]   = {};
    float     dist       = 0;
    bool      valid      = false;
};

inline std::vector<Entity> read_entities(const FFState& st) {
    std::vector<Entity> result;
    if (!st.ready) return result;

    // Walk GameObjectManagerGlobal -> tagged objects
    uint64_t mgr = 0;
    if (!rt_read(st.drv_fd, st.ff_pid, st.il2cpp + FF::OBJ_MANAGER, &mgr, 8)) return result;
    if (!mgr) return result;

    uint64_t list = 0;
    rt_read(st.drv_fd, st.ff_pid, mgr + FF::TAGGED_OBJ_LIST, &list, 8);
    if (!list) return result;

    uint32_t count = 0;
    rt_read(st.drv_fd, st.ff_pid, list + 0x8, &count, 4);
    if (count > 128) count = 128; // cap

    for (uint32_t i = 0; i < count; i++) {
        uint64_t go = 0;
        rt_read(st.drv_fd, st.ff_pid, list + 0x10 + i * 8, &go, 8);
        if (!go) continue;

        Entity e;
        e.ptr = go;

        // Read position via transform
        uint64_t comp_list = 0;
        rt_read(st.drv_fd, st.ff_pid, go + FF::COMP_LIST, &comp_list, 8);
        if (!comp_list) continue;

        rt_read(st.drv_fd, st.ff_pid, comp_list + FF::TRANSFORM_POS, &e.pos, 12);
        rt_read(st.drv_fd, st.ff_pid, comp_list + FF::TRANSFORM_HEAD, &e.head_pos, 12);

        // Read CharacterController for gameplay data
        uint64_t cc = 0;
        rt_read(st.drv_fd, st.ff_pid, go + FF::CHAR_CONTROLLER, &cc, 8);
        if (!cc) continue;

        rt_read(st.drv_fd, st.ff_pid, cc + FF::CC_HEALTH, &e.hp, 4);
        rt_read(st.drv_fd, st.ff_pid, cc + FF::CC_MAX_HEALTH, &e.max_hp, 4);
        rt_read(st.drv_fd, st.ff_pid, cc + FF::CC_IS_DEAD, &e.is_dead, 1);
        rt_read(st.drv_fd, st.ff_pid, cc + FF::CC_TEAM_ID, &e.team_id, 4);

        if (e.is_dead || e.hp <= 0) continue;
        e.valid = true;
        result.push_back(e);
    }
    return result;
}
