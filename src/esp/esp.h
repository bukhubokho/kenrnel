#pragma once
#include "../ff/offsets.h"
#include "../ff/memory.h"
#include <vector>

// ImGui-based ESP drawing — call inside ImGui frame
struct ESPConfig {
    bool box_esp      = true;
    bool health_bar   = true;
    bool head_dot     = true;
    bool name_tag     = true;
    bool distance_tag = true;
    bool show_enemies = true;
    bool show_team    = false;
    float max_dist    = 300.f;
    int   my_team_id  = -1;

    // Colors (RGBA 0-255)
    uint32_t color_enemy = 0xFF4444FF;  // red (ABGR for ImGui)
    uint32_t color_team  = 0xFF44FF44;  // green
    uint32_t color_hp    = 0xFF00E000;
    uint32_t color_hp_bg = 0xFF303030;
};

struct Camera {
    float view[16];   // view matrix
    float proj[16];   // projection matrix
    int   screen_w;
    int   screen_h;
    FF::Vec3 pos;     // camera world pos
};

// Project world position to screen. Returns false if behind camera.
bool world_to_screen(const Camera& cam, const FF::Vec3& world, FF::Vec2& screen);

float dist3d(const FF::Vec3& a, const FF::Vec3& b);

// Draw all ESP elements for visible entities (call between ImGui::NewFrame / Render)
void esp_draw(const std::vector<Entity>& entities,
              const Camera& cam,
              const ESPConfig& cfg);
