#pragma once
#include "../esp/esp.h"

struct MenuState {
    bool visible     = false;
    bool aimbot      = false;
    bool silent_aim  = false;
    float aim_fov    = 60.f;
    float aim_smooth = 0.15f;
    int   aim_bone   = 0;   // 0=head, 1=chest
    bool  esp_box    = true;
    bool  esp_hp     = true;
    bool  esp_head   = true;
    bool  esp_name   = true;
    bool  esp_dist   = true;
    float max_dist   = 300.f;
    bool  enemy_only = true;
    bool  no_recoil  = false;
    bool  speed_hack = false;
    float speed_mult = 1.0f;
};

// Call once after ImGui::NewFrame() and before ImGui::Render()
// Returns false when user requests exit
bool menu_draw(MenuState& menu, ESPConfig& esp_cfg,
               const char* key_display, bool driver_ok);

void menu_apply(const MenuState& menu, ESPConfig& out_esp);
