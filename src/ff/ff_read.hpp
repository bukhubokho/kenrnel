#pragma once
#include <cstdint>
#include <cstring>

namespace ff {

struct Vec3 { float x, y, z; };

struct PlayerInfo {
    bool    valid;
    bool    is_dead;
    bool    is_bot;
    bool    is_local;
    float   hp;           // 0..100
    Vec3    head_pos;
    Vec3    foot_pos;
    char    name[32];
};

static constexpr int MAX_PLAYERS = 60;

// Call once after PID is known — sets base address
bool init(int drv_fd, uint32_t pid);

// Call every frame — fills players[], returns count (0 if game not ready)
int  update(int drv_fd, uint32_t pid, PlayerInfo* players);

// Base address of libil2cpp.so in FF process (set by init)
extern uint64_t g_base;

// Debug — last seen pointer values, for the debug tab
struct DbgChain {
    uint64_t gf_read_addr;   // g_base + GameFacade offset (address we read)
    uint32_t elf_magic;      // read 4 bytes at g_base — should be 0x464C457F if driver works
    uint64_t gf_static;
    uint64_t base_game;      // static_fields*
    uint64_t match_game;     // CurrentMatchGame
    uint64_t match;
    uint64_t local_player;
    uint64_t dict;
    int32_t  dict_count;     // NOLEDCGOJKC 0x128 count
    int32_t  dict_all_count; // NJCGFLANJLH 0x130 count
    int32_t  list_count;     // LMOIPODCHOL 0x158 count
    int32_t  list2_count;    // DPFFPGAFGDK 0x160 count
    int32_t  players_found;
    bool     match_running;
    bool     rt_set_ok;
    bool     used_heap_scan;  // true when match found via heap scan (TypeInfo chain was 0)
    // camera chain debug
    uint64_t cam_obj;
    uint64_t cam_follow;
    uint64_t cam_native;
    // matrix / position debug — set each frame to diagnose ESP clustering
    float    dbg_mvp11;   // g_mvp[11]: should be ≈ -1.0 for valid perspective VP
    float    dbg_mvp15;   // g_mvp[15]: should be ≈  1.0
    Vec3     dbg_pos0;    // head_pos of first found enemy (should be non-zero world pos)
};
extern DbgChain g_dbg;

} // namespace ff
