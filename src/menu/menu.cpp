#include "menu.h"
#include "../imgui/imgui.h"
#include <cstdio>
#include <cstring>

// Internal tab state
static int s_tab = 0;

static void styled_toggle(const char* label, bool* v) {
    ImVec2 sz = {40.f, 20.f};
    ImVec2 p  = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    bool clicked = ImGui::InvisibleButton(label, sz);
    if (clicked) *v = !*v;

    ImU32 bg = *v ? IM_COL32(100, 220, 130, 255) : IM_COL32(60, 60, 70, 255);
    float knob_x = *v ? p.x + sz.x - 11.f : p.x + 11.f;
    dl->AddRectFilled(p, {p.x+sz.x, p.y+sz.y}, bg, sz.y*0.5f);
    dl->AddCircleFilled({knob_x, p.y+sz.y*0.5f}, 8.f, IM_COL32(255,255,255,240));

    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.f);
    ImGui::Text("%s", label);
}

static void section_header(const char* title) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(160, 200, 255, 255));
    ImGui::Text("— %s", title);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
}

bool menu_draw(MenuState& menu, ESPConfig& esp_cfg,
               const char* key_display, bool driver_ok) {
    if (!menu.visible) return true;

    // Professional dark theme
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding        = 10.f;
    st.FrameRounding         = 5.f;
    st.ItemSpacing           = {8.f, 6.f};
    st.WindowBorderSize      = 1.f;
    st.FramePadding          = {8.f, 4.f};
    // Allow dragging to all edges — zero out ImGui's default clamp padding
    st.DisplayWindowPadding  = {0.f, 0.f};
    st.DisplaySafeAreaPadding = {0.f, 0.f};

    ImGui::PushStyleColor(ImGuiCol_WindowBg,      IM_COL32(15, 15, 22,  245));
    ImGui::PushStyleColor(ImGuiCol_TitleBg,       IM_COL32(20, 20, 35,  255));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, IM_COL32(30, 30, 55,  255));
    ImGui::PushStyleColor(ImGuiCol_Border,        IM_COL32(80, 80, 120, 180));
    ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(40, 40, 65,  220));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(60, 80, 140, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(80,120, 200, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,       IM_COL32(25, 25, 40,  200));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,    IM_COL32(100,160, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_Header,        IM_COL32(50, 60, 100, 200));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(70, 90, 160, 220));
    ImGui::PushStyleColor(ImGuiCol_Tab,           IM_COL32(30, 30, 50,  220));
    ImGui::PushStyleColor(ImGuiCol_TabActive,     IM_COL32(70,100, 200, 255));
    ImGui::PushStyleColor(ImGuiCol_TabHovered,    IM_COL32(50, 70, 140, 240));

    ImGuiIO& sio = ImGui::GetIO();
    float sw = sio.DisplaySize.x, sh = sio.DisplaySize.y;
    float scale = sio.FontGlobalScale > 0.f ? sio.FontGlobalScale : 1.f;
    float mw = 380.f * scale, mh = 520.f * scale;
    ImGui::SetNextWindowSize({mw, mh}, ImGuiCond_Once);
    // Force-center on first show and on every rotation change.
    // Must be called AFTER ImGui::NewFrame() — NewFrame clears NextWindowData,
    // so any SetNextWindowPos from before NewFrame is lost.
    if (menu.force_center) {
        ImGui::SetNextWindowPos({(sw - mw) * 0.5f, (sh - mh) * 0.5f}, ImGuiCond_Always);
        menu.force_center = false;
    }

    bool open = true;
    ImGui::Begin("##KernelMenu", &open,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

    // Header bar
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(130, 180, 255, 255));
    ImGui::SetWindowFontScale(1.2f);
    ImGui::Text("  KERNEL");
    ImGui::PopStyleColor();
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 90.f);

    // Status indicator
    if (driver_ok) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(80, 220, 80, 255));
        ImGui::Text("⬤ ACTIVE");
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 60, 60, 255));
        ImGui::Text("⬤ NO DRV");
    }
    ImGui::PopStyleColor();
    ImGui::Separator();

    // Key display (truncated)
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120,120,120,200));
    ImGui::Text("  Key: %.16s...", key_display ? key_display : "none");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Tab bar
    if (ImGui::BeginTabBar("##tabs")) {
        if (ImGui::BeginTabItem("ESP")) {
            s_tab = 0;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("AIM")) {
            s_tab = 1;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("MISC")) {
            s_tab = 2;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Spacing();

    if (s_tab == 0) {
        // ESP tab
        section_header("Visuals");
        styled_toggle("Box ESP",    &menu.esp_box);
        styled_toggle("Health Bar", &menu.esp_hp);
        styled_toggle("Head Dot",   &menu.esp_head);
        styled_toggle("Name Tag",   &menu.esp_name);
        styled_toggle("Distance",   &menu.esp_dist);
        styled_toggle("Enemy Only", &menu.enemy_only);
        ImGui::Spacing();
        ImGui::Text("Max Distance");
        ImGui::SliderFloat("##dist", &menu.max_dist, 50.f, 500.f, "%.0f m");
    }
    else if (s_tab == 1) {
        // AIM tab
        section_header("Aimbot");
        styled_toggle("Aimbot",     &menu.aimbot);
        styled_toggle("Silent Aim", &menu.silent_aim);
        ImGui::Spacing();
        ImGui::Text("FOV");
        ImGui::SliderFloat("##fov", &menu.aim_fov, 5.f, 180.f, "%.0f°");
        ImGui::Text("Smoothing");
        ImGui::SliderFloat("##smooth", &menu.aim_smooth, 0.01f, 1.f, "%.2f");
        ImGui::Spacing();
        section_header("Target Bone");
        if (ImGui::RadioButton("Head",  menu.aim_bone == 0)) menu.aim_bone = 0;
        ImGui::SameLine();
        if (ImGui::RadioButton("Chest", menu.aim_bone == 1)) menu.aim_bone = 1;
        ImGui::Spacing();
        styled_toggle("No Recoil", &menu.no_recoil);
    }
    else {
        // MISC tab
        section_header("Movement");
        styled_toggle("Speed Hack", &menu.speed_hack);
        if (menu.speed_hack) {
            ImGui::Text("Speed Multiplier");
            ImGui::SliderFloat("##spd", &menu.speed_mult, 1.f, 3.f, "%.1fx");
        }
        ImGui::Spacing();
        section_header("Info");
        ImGui::Text("Driver:  %s", driver_ok ? "Connected" : "Offline");
        ImGui::Text("Overlay: OpenGL ES 2.0");
    }

    ImGui::End();
    ImGui::PopStyleColor(14);

    if (!open) return false;
    return true;
}

void menu_apply(const MenuState& menu, ESPConfig& esp) {
    esp.box_esp      = menu.esp_box;
    esp.health_bar   = menu.esp_hp;
    esp.head_dot     = menu.esp_head;
    esp.name_tag     = menu.esp_name;
    esp.distance_tag = menu.esp_dist;
    esp.show_enemies = menu.enemy_only ? true : true;
    esp.show_team    = !menu.enemy_only;
    esp.max_dist     = menu.max_dist;
}
