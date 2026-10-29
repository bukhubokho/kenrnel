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

} // namespace ff
