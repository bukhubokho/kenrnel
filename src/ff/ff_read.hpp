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
    uint64_t gf_static;
    uint64_t base_game;      // static_fields*  (gf_static + 0xB8)
    uint64_t match_game;     // CurrentMatchGame (static_fields + 0x8)
    uint64_t match;
    uint64_t local_player;
    uint64_t dict;
    int32_t  dict_count;
    int32_t  players_found;
    bool     match_running;
    bool     rt_set_ok;      // rt_set_pid succeeded
};
extern DbgChain g_dbg;

} // namespace ff
