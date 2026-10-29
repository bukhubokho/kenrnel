// language: C++17, target: Android arm64
// Reads Free Fire entity list via RTkernel driver (ring-0, no injection).
// All pointer chains are OB55 offsets from Offsets.h.
#include "ff_read.hpp"
#include "Offsets.h"
#include "../driver/driver.h"
#include "ff_pid.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace ff {

uint64_t g_base = 0;
DbgChain g_dbg  = {};

// ── Helpers ──────────────────────────────────────────────────────────────────
template<typename T>
static inline T rd(int fd, uint32_t pid, uint64_t addr) {
    return rt_read_t<T>(fd, pid, addr);
}

// Read UTF-16 il2cpp string → ASCII (first 31 chars)
static void read_il2cpp_str(int fd, uint32_t pid, uint64_t str_ptr, char* out, int max) {
    out[0] = '\0';
    if (!str_ptr) return;
    int32_t len = rd<int32_t>(fd, pid, str_ptr + 0x10);
    if (len <= 0 || len > 64) return;
    if (len >= max) len = max - 1;
    uint16_t wbuf[64] = {};
    rt_read(fd, pid, str_ptr + 0x14, wbuf, (size_t)len * 2);
    for (int i = 0; i < len; i++)
        out[i] = (char)(wbuf[i] & 0x7F);
    out[len] = '\0';
}

// Read Unity Transform world position (offset 0x90 = cached world pos on OB55)
static Vec3 read_transform_pos(int fd, uint32_t pid, uint64_t tf_ptr) {
    Vec3 v{};
    if (!tf_ptr) return v;
    v.x = rd<float>(fd, pid, tf_ptr + 0x90);
    v.y = rd<float>(fd, pid, tf_ptr + 0x94);
    v.z = rd<float>(fd, pid, tf_ptr + 0x98);
    return v;
}

// ── Find libil2cpp.so base from /proc/<pid>/maps ──────────────────────────────
static uint64_t find_il2cpp_base(uint32_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%u/maps", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    char line[256];
    uint64_t base = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "libil2cpp.so")) {
            uint64_t addr = (uint64_t)strtoull(line, nullptr, 16);
            if (base == 0 || addr < base) base = addr;
        }
    }
    fclose(f);
    return base;
}

// ── Init ─────────────────────────────────────────────────────────────────────
bool init(int drv_fd, uint32_t pid) {
    if (drv_fd < 0 || pid == 0) return false;

    g_dbg.rt_set_ok = rt_set_pid_ok(drv_fd, pid, PACKAGE);
    uint64_t drv_base = rt_set_pid(drv_fd, pid, PACKAGE);

    uint64_t base = find_il2cpp_base(pid);
    if (!base) return false;
    g_base = base;
    return true;
}

// ── Update — walk entity dict each frame ─────────────────────────────────────
int update(int drv_fd, uint32_t pid, PlayerInfo* players) {
    using namespace Offsets;

    g_dbg = {};
    if (drv_fd < 0 || pid == 0) return 0;

    // Refresh SET_PID every frame — some RTkernel builds expire the mm ref
    g_dbg.rt_set_ok = rt_set_pid_ok(drv_fd, pid, PACKAGE);

    // libil2cpp.so can have multiple mappings — only use one with valid ELF header
    uint64_t fresh_base = find_il2cpp_base(pid);
    if (fresh_base) {
        uint32_t magic = rd<uint32_t>(drv_fd, pid, fresh_base);
        if (magic == 0x464C457F) g_base = fresh_base;
    }
    if (!g_base) return 0;

    // Sanity: read ELF magic at g_base to verify driver is reading correctly
    g_dbg.elf_magic    = rd<uint32_t>(drv_fd, pid, g_base);
    g_dbg.gf_read_addr = g_base + GameFacade;

    // Scan 8 x uint64 around GameFacade offset to find non-zero TypeInfo
    for (int i = 0; i < 8; i++)
        g_dbg.scan[i] = rd<uint64_t>(drv_fd, pid, g_base + GameFacade - 0x10 + (uint64_t)i * 8);

    // Step 1: read Il2CppClass* (TypeInfo) from g_base + GameFacade RVA
    uint64_t gf_static = rd<uint64_t>(drv_fd, pid, g_base + GameFacade);
    g_dbg.gf_static = gf_static;
    if (!gf_static) return 0;

    // Step 2: Il2CppClass* + 0xB8 = static_fields*
    uint64_t base_game = rd<uint64_t>(drv_fd, pid, gf_static + GameFacade_SF);
    g_dbg.base_game = base_game;
    if (!base_game) return 0;

    // Step 3: static_fields + 0x8 = CurrentMatchGame (MatchGame*)
    uint64_t match_game = rd<uint64_t>(drv_fd, pid, base_game + GameFacade_MatchGame);
    g_dbg.match_game = match_game;
    if (!match_game) return 0;

    // MatchGame + 0x90 = m_Match
    uint64_t match = rd<uint64_t>(drv_fd, pid, match_game + BaseGame_Match);
    g_dbg.match = match;
    if (!match) return 0;

    // Check match running
    uint8_t running = rd<uint8_t>(drv_fd, pid, match + MatchIsRunning);
    g_dbg.match_running = running != 0;
    if (!running) return 0;

    uint64_t local_player = rd<uint64_t>(drv_fd, pid, match + Match_LocalPlayer);
    g_dbg.local_player = local_player;

    // Player dictionary
    uint64_t dict = rd<uint64_t>(drv_fd, pid, match + Match_PlayerDict);
    g_dbg.dict = dict;
    if (!dict) return 0;

    uint64_t entries_arr = rd<uint64_t>(drv_fd, pid, dict + Dict_Entries);
    if (!entries_arr) return 0;

    int32_t count = rd<int32_t>(drv_fd, pid, dict + Dict_Count);
    g_dbg.dict_count = count;
    if (count <= 0 || count > MAX_PLAYERS) return 0;

    // entries data starts after il2cpp array header (Dict_ArrayHeader = 0x20)
    uint64_t data = entries_arr + Dict_ArrayHeader;

    int found = 0;
    for (int i = 0; i < count && found < MAX_PLAYERS; i++) {
        uint64_t entry_base = data + (uint64_t)i * Dict_EntryStride;
        uint64_t player     = rd<uint64_t>(drv_fd, pid, entry_base + Dict_EntryValue);
        if (!player) continue;

        PlayerInfo& p = players[found];
        memset(&p, 0, sizeof(p));

        p.is_dead  = rd<uint8_t>(drv_fd, pid, player + Player_IsDead) != 0;
        if (p.is_dead) continue;

        p.is_local = (player == local_player);
        p.is_bot   = rd<uint8_t>(drv_fd, pid, player + Player_IsBot) != 0;

        // HP — chain: player + Player_HP → list ptr → entry → value
        uint64_t hp_list = rd<uint64_t>(drv_fd, pid, player + Player_HP);
        if (hp_list) {
            uint64_t hp_entry = rd<uint64_t>(drv_fd, pid, hp_list + HP_Entry);
            if (hp_entry) {
                p.hp = rd<float>(drv_fd, pid, hp_entry + HP_Value);
            }
        }
        if (p.hp <= 0.f) continue;

        // Positions
        uint64_t head_tf = rd<uint64_t>(drv_fd, pid, player + Player_HeadTF);
        uint64_t foot_tf = rd<uint64_t>(drv_fd, pid, player + Player_FootTF);
        p.head_pos = read_transform_pos(drv_fd, pid, head_tf);
        p.foot_pos = read_transform_pos(drv_fd, pid, foot_tf);

        if (p.head_pos.x == 0.f && p.head_pos.y == 0.f) continue;

        // Name
        uint64_t name_str = rd<uint64_t>(drv_fd, pid, player + Player_Name);
        read_il2cpp_str(drv_fd, pid, name_str, p.name, sizeof(p.name));

        p.valid = true;
        found++;
    }
    g_dbg.players_found = found;
    return found;
}

} // namespace ff
