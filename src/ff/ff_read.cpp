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
#include <ctime>

namespace ff {

uint64_t g_base = 0;
DbgChain g_dbg  = {};
static int64_t s_last_scan_ms = 0;

// ── Heap-scan state ───────────────────────────────────────────────────────────
struct HeapRange { uint64_t start, size; };
static HeapRange s_heap[64];
static int       s_heap_n         = 0;
static int64_t   s_heap_ranges_ms = 0;   // when we last parsed maps for heap
static uint64_t  s_heap_match     = 0;   // cached match ptr from heap scan
static int       s_heap_ri        = 0;   // current range index
static uint64_t  s_heap_off       = 0;   // current offset within that range
static int64_t   s_heap_scan_ms   = 0;   // last scan tick

static int64_t now_ms() {
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

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

// ITransformNode (Player.BFOONIGAIFK) → Transform backing at +0x10
// → managed Transform m_CachedPtr (native) at +0x10
// → native C++ TransformInternal world pos at +0x90 (Unity 2021 ARM64)
static Vec3 read_transform_pos(int fd, uint32_t pid, uint64_t itf_ptr) {
    Vec3 v{};
    if (!itf_ptr) return v;
    uint64_t tf = rd<uint64_t>(fd, pid, itf_ptr + 0x10); // ITransformNode → managed Transform
    if (!tf) return v;
    uint64_t np = rd<uint64_t>(fd, pid, tf + 0x10);      // managed Transform → native ptr
    if (!np) return v;
    v.x = rd<float>(fd, pid, np + 0x90);
    v.y = rd<float>(fd, pid, np + 0x94);
    v.z = rd<float>(fd, pid, np + 0x98);
    return v;
}

// ── Parse /proc/<pid>/maps for large anonymous rw- heap ranges ───────────────
static void refresh_heap_ranges(uint32_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%u/maps", pid);
    FILE* f = fopen(path, "r");
    if (!f) return;

    s_heap_n = 0;
    char line[256];
    while (fgets(line, sizeof(line), f) && s_heap_n < 64) {
        if (!strstr(line, " rw-p ")) continue;
        // Skip named mappings (stack, libs, ashmem, etc.) — keep anonymous
        // Last non-space token: if it starts with '/' or '[' it's named
        char* nl = strchr(line, '\n'); if (nl) *nl = 0;
        // Find the 6th field (pathname) — after the 5th space
        char* p = line; int sp = 0;
        while (*p && sp < 5) { if (*p++ == ' ') { sp++; while (*p == ' ') p++; } }
        // p now points to pathname (or end of line)
        if (*p == '/' || *p == '[') continue;  // skip named

        uint64_t start, end;
        if (sscanf(line, "%lx-%lx", &start, &end) != 2) continue;
        uint64_t sz = end - start;
        if (sz < 0x200000) continue;   // skip small (<2MB) mappings
        s_heap[s_heap_n++] = {start, sz};
    }
    fclose(f);
}

// ── Heap scan: find Match object when TypeInfo chain returns 0 ───────────────
// Advances scan position 1000 candidates (0x1000 stride) per call.
// Returns cached/found match ptr, or 0.
static uint64_t heap_scan_for_match(int fd, uint32_t pid) {
    using namespace Offsets;

    // Validate cached
    if (s_heap_match) {
        uint8_t run = rd<uint8_t>(fd, pid, s_heap_match + MatchIsRunning);
        if (run == 1) return s_heap_match;
        s_heap_match = 0;
        s_heap_ri    = 0;
        s_heap_off   = 0;
    }

    int64_t now = now_ms();
    if (now - s_heap_scan_ms < 100) return 0;  // 100ms between scan ticks
    s_heap_scan_ms = now;

    // Refresh range list every 3s or on first call
    if (s_heap_n == 0 || now - s_heap_ranges_ms > 3000) {
        refresh_heap_ranges(pid);
        s_heap_ranges_ms = now;
        s_heap_ri  = 0;
        s_heap_off = 0;
    }
    if (s_heap_n == 0) return 0;

    // Scan 1000 candidates this tick (0x1000 stride)
    constexpr uint64_t STRIDE       = 0x1000;
    constexpr int      CANDS_PER_MS = 1000;

    for (int i = 0; i < CANDS_PER_MS; i++) {
        if (s_heap_ri >= s_heap_n) { s_heap_ri = 0; s_heap_off = 0; break; } // full lap done

        HeapRange& hr = s_heap[s_heap_ri];
        if (s_heap_off + 0x2500 >= hr.size) {  // exhausted this range
            s_heap_ri++;
            s_heap_off = 0;
            continue;
        }

        uint64_t candidate = hr.start + s_heap_off;
        s_heap_off += STRIDE;

        // Fast filter: MatchIsRunning == 1
        uint8_t run = rd<uint8_t>(fd, pid, candidate + MatchIsRunning);
        if (run != 1) continue;

        // LocalPlayer must be a non-null pointer (looks like heap addr)
        uint64_t lp = rd<uint64_t>(fd, pid, candidate + Match_LocalPlayer);
        if (lp < 0x100000 || (lp & 7)) continue;

        // PlayerDict must be non-null
        uint64_t dict = rd<uint64_t>(fd, pid, candidate + Match_PlayerDict);
        if (dict < 0x100000 || (dict & 7)) continue;

        // Dict count sanity
        int32_t cnt = rd<int32_t>(fd, pid, dict + Dict_Count);
        if (cnt <= 0 || cnt > MAX_PLAYERS) continue;

        s_heap_match = candidate;
        return s_heap_match;
    }
    return 0;
}

// ── Find libil2cpp.so base from /proc/<pid>/maps ──────────────────────────────
// Returns all candidate start addresses (up to 16) that map libil2cpp.so
static int find_il2cpp_candidates(uint32_t pid, uint64_t* out, int max) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%u/maps", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < max) {
        if (strstr(line, "libil2cpp.so")) {
            uint64_t addr = (uint64_t)strtoull(line, nullptr, 16);
            out[count++] = addr;
        }
    }
    fclose(f);
    return count;
}

// ── Init ─────────────────────────────────────────────────────────────────────
bool init(int drv_fd, uint32_t pid) {
    if (drv_fd < 0 || pid == 0) return false;

    g_dbg.rt_set_ok = rt_set_pid_ok(drv_fd, pid, PACKAGE);
    rt_set_pid(drv_fd, pid, PACKAGE);

    uint64_t candidates[16];
    int n = find_il2cpp_candidates(pid, candidates, 16);
    for (int i = 0; i < n; i++) {
        uint32_t magic = rd<uint32_t>(drv_fd, pid, candidates[i]);
        if (magic == 0x464C457F) { g_base = candidates[i]; break; }
    }
    return g_base != 0;
}

// ── Update — walk entity dict each frame ─────────────────────────────────────
int update(int drv_fd, uint32_t pid, PlayerInfo* players) {
    using namespace Offsets;

    g_dbg = {};
    if (drv_fd < 0 || pid == 0) return 0;

    // Refresh SET_PID every frame — some RTkernel builds expire the mm ref
    g_dbg.rt_set_ok = rt_set_pid_ok(drv_fd, pid, PACKAGE);

    // Re-scan maps when g_base is missing or ELF magic fails, rate-limit to 300ms
    if (!g_base || rd<uint32_t>(drv_fd, pid, g_base) != 0x464C457F) {
        int64_t now = now_ms();
        if (now - s_last_scan_ms >= 300) {
            s_last_scan_ms = now;
            uint64_t candidates[16];
            int n = find_il2cpp_candidates(pid, candidates, 16);
            for (int i = 0; i < n; i++) {
                uint32_t magic = rd<uint32_t>(drv_fd, pid, candidates[i]);
                if (magic == 0x464C457F) { g_base = candidates[i]; break; }
            }
        }
    }
    if (!g_base) return 0;

    // Sanity: read ELF magic at g_base to verify driver is reading correctly
    g_dbg.elf_magic    = rd<uint32_t>(drv_fd, pid, g_base);
    g_dbg.gf_read_addr = g_base + GameFacade;

    // Step 1: read Il2CppClass* (TypeInfo) from g_base + GameFacade RVA
    uint64_t gf_static = rd<uint64_t>(drv_fd, pid, g_base + GameFacade);
    g_dbg.gf_static = gf_static;

    uint64_t match = 0;
    g_dbg.used_heap_scan = false;

    if (!gf_static) {
        // TypeInfo not yet initialized — try heap scan fast-path
        uint64_t heap_match = heap_scan_for_match(drv_fd, pid);
        if (!heap_match) return 0;
        match = heap_match;
        g_dbg.match     = match;
        g_dbg.used_heap_scan = true;
    } else {
        // Step 2: Il2CppClass* + 0xB8 = static_fields*
        uint64_t base_game = rd<uint64_t>(drv_fd, pid, gf_static + GameFacade_SF);
        g_dbg.base_game = base_game;
        if (!base_game) return 0;

        // Step 3: static_fields + 0x8 = CurrentMatchGame (MatchGame*)
        uint64_t match_game = rd<uint64_t>(drv_fd, pid, base_game + GameFacade_MatchGame);
        g_dbg.match_game = match_game;
        if (!match_game) return 0;

        // MatchGame + 0x90 = m_Match
        match = rd<uint64_t>(drv_fd, pid, match_game + BaseGame_Match);
        g_dbg.match = match;
        if (!match) return 0;
    }

    // Check match running (informational only — training mode has running=0 but valid match ptr)
    uint8_t running = rd<uint8_t>(drv_fd, pid, match + MatchIsRunning);
    g_dbg.match_running = running != 0;

    uint64_t local_player = rd<uint64_t>(drv_fd, pid, match + Match_LocalPlayer);
    g_dbg.local_player = local_player;

    // Probe all player collections — record counts for debug
    uint64_t dict     = rd<uint64_t>(drv_fd, pid, match + Match_PlayerDict);
    uint64_t dict_all = rd<uint64_t>(drv_fd, pid, match + Match_PlayerDictAll);
    uint64_t list_ptr = rd<uint64_t>(drv_fd, pid, match + Match_PlayerList);
    uint64_t list2_ptr= rd<uint64_t>(drv_fd, pid, match + Match_PlayerList2);
    g_dbg.dict = dict;

    int32_t cnt_dict  = dict     ? rd<int32_t>(drv_fd, pid, dict      + Dict_Count) : 0;
    int32_t cnt_dall  = dict_all ? rd<int32_t>(drv_fd, pid, dict_all  + Dict_Count) : 0;
    int32_t cnt_list  = list_ptr ? rd<int32_t>(drv_fd, pid, list_ptr  + List_Size)  : 0;
    int32_t cnt_list2 = list2_ptr? rd<int32_t>(drv_fd, pid, list2_ptr + List_Size)  : 0;
    g_dbg.dict_count  = cnt_dict;
    g_dbg.dict_all_count = cnt_dall;
    g_dbg.list_count  = cnt_list;
    g_dbg.list2_count = cnt_list2;

    // Pick the source with the most players (most likely all-players)
    // Prefer List (0x158 > 0x160) over Dict when counts differ
    enum SrcKind { SRC_LIST, SRC_LIST2, SRC_DICT_ALL, SRC_DICT } src;
    int32_t max_count = 0;
    src = SRC_DICT; max_count = cnt_dict;
    if (cnt_dall  > max_count) { src = SRC_DICT_ALL; max_count = cnt_dall; }
    if (cnt_list2 > max_count) { src = SRC_LIST2;    max_count = cnt_list2; }
    if (cnt_list  > max_count) { src = SRC_LIST;     max_count = cnt_list; }

    if (max_count <= 0 || max_count > MAX_PLAYERS) return 0;

    // Build flat array of player ptrs from the best source
    uint64_t ptrs[MAX_PLAYERS] = {};
    int ptrs_n = 0;

    if (src == SRC_LIST || src == SRC_LIST2) {
        uint64_t lp   = (src == SRC_LIST) ? list_ptr : list2_ptr;
        uint64_t items= rd<uint64_t>(drv_fd, pid, lp + List_Items);
        if (items) {
            uint64_t base_elem = items + 0x20; // array header 0x20
            for (int i = 0; i < max_count; i++)
                ptrs[ptrs_n++] = rd<uint64_t>(drv_fd, pid, base_elem + (uint64_t)i * 8);
        }
    } else {
        // Dict source (SRC_DICT or SRC_DICT_ALL)
        uint64_t dp = (src == SRC_DICT_ALL) ? dict_all : dict;
        uint64_t entries_arr = dp ? rd<uint64_t>(drv_fd, pid, dp + Dict_Entries) : 0;
        if (entries_arr) {
            uint64_t data = entries_arr + Dict_ArrayHeader;
            for (int i = 0; i < max_count; i++)
                ptrs[ptrs_n++] = rd<uint64_t>(drv_fd, pid, data + (uint64_t)i * Dict_EntryStride + Dict_EntryValue);
        }
    }

    int found = 0;
    for (int i = 0; i < ptrs_n && found < MAX_PLAYERS; i++) {
        uint64_t player = ptrs[i];
        if (!player) continue;

        PlayerInfo& p = players[found];
        memset(&p, 0, sizeof(p));

        p.is_dead  = rd<uint8_t>(drv_fd, pid, player + Player_IsDead) != 0;
        if (p.is_dead) continue;

        // FollowCamera only exists on the local player — more reliable than ptr comparison
        p.is_local = (rd<uint64_t>(drv_fd, pid, player + Player_Camera) != 0);
        p.is_bot   = rd<uint8_t>(drv_fd, pid, player + Player_IsBot) != 0;

        // HP — chain: player+Player_HP → PRIDataPool → m_Datas(+HP_List) → entry[0](+HP_Entry) → Value(+HP_Value)
        bool hp_ok = false;
        uint64_t hp_pool = rd<uint64_t>(drv_fd, pid, player + Player_HP);
        if (hp_pool) {
            uint64_t hp_datas = rd<uint64_t>(drv_fd, pid, hp_pool + HP_List);
            if (hp_datas) {
                uint64_t hp_entry = rd<uint64_t>(drv_fd, pid, hp_datas + HP_Entry);
                if (hp_entry) {
                    p.hp = rd<float>(drv_fd, pid, hp_entry + HP_Value);
                    hp_ok = true;
                }
            }
        }
        // Only skip if we got a real HP read and it's zero (actually dead)
        if (hp_ok && p.hp <= 0.f) continue;
        if (!hp_ok) p.hp = 100.f;  // unknown — assume alive

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
