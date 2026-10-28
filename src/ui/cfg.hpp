#pragma once
#include "imgui.h"

// ─────────────────────────────────────────────────────────────────────────────
//  cfg.hpp  —  Central config namespace
//  Все inline-переменные — единственная точка истины для всего чита.
//  SyncMenuCfg() в antilose_compat.cpp зеркалит Menu::cfg → cfg::*
// ─────────────────────────────────────────────────────────────────────────────

namespace cfg {

// ── ESP ───────────────────────────────────────────────────────────────────────
namespace esp {
    inline bool  box        = true;
    inline bool  name       = true;
    inline bool  weapon     = true;
    inline bool  health     = true;
    inline bool  distance   = false;
    inline bool  snapline   = false;
    inline bool  headdot    = false;
    inline bool  skeleton   = false;
    inline int   box_type   = 0;        // 0=2D 1=Corner 2=3D
    inline float box_rounding = 0.f;
    inline float max_dist   = 300.f;
    inline ImVec4 box_col      = {1.f,1.f,1.f,1.f};
    inline ImVec4 name_col     = {1.f,1.f,1.f,1.f};
    inline ImVec4 weapon_col   = {1.f,1.f,1.f,1.f};
    inline ImVec4 health_col   = {1.f,1.f,1.f,1.f};
    inline ImVec4 distance_col = {1.f,1.f,1.f,1.f};
    inline ImVec4 snap_col     = {1.f,1.f,1.f,.8f};
    inline ImVec4 head_col     = {1.f,1.f,1.f,1.f};
    inline ImVec4 skel_col     = {.3f,1.f,1.f,.9f};
    // health bar gradient colors (top/bot)
    inline ImVec4 hp_top_col   = {.2f,1.f,.2f,1.f};
    inline ImVec4 hp_bot_col   = {1.f,.2f,.2f,1.f};
    // distance-based fade: ESP fades when enemy > fade_dist
    inline bool  fade_dist_enabled = false;
    inline float fade_dist         = 200.f;
}

// ── Aimbot ────────────────────────────────────────────────────────────────────
namespace aim {
    inline bool  enabled       = false;
    inline bool  hard_lock     = true;
    inline bool  silent        = false;
    inline bool  visible_check = false;
    inline bool  team_check    = true;
    inline bool  draw_fov      = false;
    inline float fov           = 30.f;
    inline float smooth        = 5.f;
    inline float max_dist      = 300.f;
    inline int   bone          = 0;     // 0=Head 1=Neck 2=Chest 3=Stomach 4=Pelvis
    inline int   target_mode   = 0;     // 0=ClosestFOV 1=LowHP 2=Distance
}

// ── Mini Radar ────────────────────────────────────────────────────────────────
namespace radar {
    inline bool  enabled   = false;
    inline float size      = 150.f;
    inline float range     = 80.f;
    inline float pos_x     = 10.f;
    inline float pos_y     = 10.f;
    inline bool  show_dist = true;  // цвет точки по дистанции
    inline int   style     = 0;     // 0=Round 1=Square
}

// ── World ─────────────────────────────────────────────────────────────────────
namespace world {
    inline bool  potato    = false; // Potato GFX
    inline bool  fullbright= false;
    inline int   time_of_day = 12; // 0..24
}

// ── Night Mode ────────────────────────────────────────────────────────────────
namespace nightmode {
    inline bool enabled = false;
}

// ── Ambient Light ─────────────────────────────────────────────────────────────
namespace ambient {
    inline bool   enabled   = false;
    inline ImVec4 color     = {0.05f,0.05f,0.2f,1.f};
    inline float  intensity = 0.12f;
    inline bool   pulse     = true;
    inline float  pulse_spd = 1.5f;
}

// ── Bullet Tracers ────────────────────────────────────────────────────────────
namespace tracer {
    inline bool   enabled   = false;
    inline ImVec4 color     = {1.f,0.4f,0.1f,1.f};
    inline float  thickness = 1.5f;
    inline float  lifetime  = 1.2f;
    inline bool   spark     = true;
    inline bool   fade      = true;
    inline int    mode      = 0;  // 0=2D 1=3D
}

// ── Weapon ────────────────────────────────────────────────────────────────────
namespace weapon {
    inline bool norecoil  = false;
    inline bool nospread  = false;
    inline bool rapidfire = false;
}

// ── Misc ──────────────────────────────────────────────────────────────────────
namespace misc {
    inline bool watermark   = true;
    inline bool fps_counter = false;
    inline bool streamproof = false;
    inline int  fps_limit   = 1;    // 0=30 1=60 2=90 3=120 4=144
    // Crosshair overlay
    inline bool  crosshair      = false;
    inline int   crosshair_style= 0;    // 0=Cross 1=Dot 2=Circle 3=Cross+Circle
    inline float crosshair_size = 8.f;
    inline float crosshair_gap  = 4.f;
    inline float crosshair_thick= 1.5f;
    inline ImVec4 crosshair_col = {1.f,1.f,1.f,1.f};
    // Hit marker (on HP drop)
    inline bool  hitmarker      = false;
    inline float hitmarker_size = 10.f;
    inline ImVec4 hitmarker_col = {1.f,0.3f,0.1f,1.f};
    inline float hitmarker_dur  = 0.35f;
    // Rain
    inline bool  rain         = false;
    inline float rain_opacity = 0.6f;
    // Speedhack
    inline bool  speedhack      = false;
    inline int   speed_mult     = 15;   // /10 → float 1.0..3.0
}

// ── Effects ─────────────────────────────────────────────────────────────────
namespace effects {
    inline bool hit_particles = true;  // blood/spark burst on hit
    inline bool death_effect  = true;  // explosion particles on kill
    inline bool death_ring    = true;  // expanding ring on kill
}

// ── ESP Gradient & Glow ───────────────────────────────────────────────────────
namespace esp_extra {
    inline bool   gradient       = true;
    inline bool   visible_color  = true;
    inline ImVec4 grad_near      = {0.2f, 0.85f, 1.f, 1.f};
    inline ImVec4 grad_far       = {1.f,  0.18f, 0.18f, 1.f};
    inline ImVec4 visible_col    = {0.3f, 1.f,   0.3f, 1.f};
    inline bool   glow           = false;
    inline float  glow_size      = 8.f;
    inline float  glow_alpha     = 0.40f;
}

// ── Settings ──────────────────────────────────────────────────────────────────
namespace settings {
    inline bool bone_size = false;
    inline bool auto_win  = false;
    inline bool hit_logs  = false;
    inline bool hit_sound = false;
    inline int  hit_sound_type = 0; // 0=Click 1=Bell 2=Pop
    // Triggerbot
    inline bool triggerbot       = false;
    inline bool tb_visible_check = true;
    inline bool tb_only_ads      = false;
    inline int  tb_delay         = 80;   // ms задержка перед выстрелом
    inline int  tb_interval      = 120;  // ms между выстрелами (RPM limit)
    inline bool tb_tap_fire = false;
    inline int tb_fire_x_pct = 90;
    inline int tb_fire_y_pct = 80;
    // Item ESP
    inline bool item_esp   = false;
    inline bool weapons    = true;
    inline bool ammo       = false;
    inline bool medkits    = false;
    inline int  item_dist  = 150;
    // Throwables
    inline bool grenade_warn= true;
    inline bool trajectory  = false;
    inline int  prediction  = 1;    // 0=Low 1=Med 2=High
}

} // namespace cfg
