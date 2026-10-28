#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include <cstdio>
#include <cstring>
#include <cmath>

// ── Wolsaw-style menu (NexuraWare dark theme) ────────────────────────────────
// Style + layout taken from Wolsaw/src/ui/menu.cpp.
// All project logic (ESP, aimbot, driver, overlay) untouched.

static int   s_tab       = 0;
static float s_alpha     = 0.f;
static bool  s_style_ok  = false;

static const float TITLE_H   = 52.f;
static const float TABS_H    = 60.f;
static const float CONTENT_OFF = TITLE_H + TABS_H + 30.f;

// ── Style ────────────────────────────────────────────────────────────────────
static void init_style() {
    if (s_style_ok) return;
    ImGuiStyle& s = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    s.WindowBorderSize  = 1.f; s.ChildBorderSize  = 1.f;
    s.FrameBorderSize   = 1.f; s.WindowRounding   = 0.f;
    s.ChildRounding     = 0.f; s.FrameRounding    = 0.f;
    s.PopupRounding     = 0.f; s.ScrollbarRounding= 0.f;
    s.GrabRounding      = 0.f;
    s.WindowPadding     = ImVec2(16.f, 16.f);
    s.FramePadding      = ImVec2(8.f,  4.f);
    s.ItemSpacing       = ImVec2(12.f, 10.f);
    s.ScrollbarSize     = 10.f;
    s.Colors[ImGuiCol_Text]                  = ImVec4(1.f,  1.f,  1.f,  1.f);
    s.Colors[ImGuiCol_TextDisabled]          = ImVec4(.5f,  .5f,  .5f,  1.f);
    s.Colors[ImGuiCol_WindowBg]              = ImVec4(.04f, .04f, .04f, 1.f);
    s.Colors[ImGuiCol_ChildBg]               = ImVec4(.06f, .06f, .06f, 1.f);
    s.Colors[ImGuiCol_PopupBg]               = ImVec4(.08f, .08f, .08f, .94f);
    s.Colors[ImGuiCol_Border]                = ImVec4(0.f,  0.f,  0.f,  1.f);
    s.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.f,  0.f,  0.f,  0.f);
    s.Colors[ImGuiCol_FrameBg]               = ImVec4(.04f, .04f, .04f, 1.f);
    s.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(.07f, .07f, .07f, 1.f);
    s.Colors[ImGuiCol_FrameBgActive]         = ImVec4(.10f, .10f, .10f, 1.f);
    s.Colors[ImGuiCol_TitleBg]               = ImVec4(.04f, .04f, .04f, 1.f);
    s.Colors[ImGuiCol_TitleBgActive]         = ImVec4(.16f, .29f, .48f, 1.f);
    s.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(.02f, .02f, .02f, .53f);
    s.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(.31f, .31f, .31f, 1.f);
    s.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(.41f, .41f, .41f, 1.f);
    s.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(.51f, .51f, .51f, 1.f);
    s.Colors[ImGuiCol_CheckMark]             = ImVec4(1.f,  1.f,  1.f,  1.f);
    s.Colors[ImGuiCol_SliderGrab]            = ImVec4(1.f,  1.f,  1.f,  1.f);
    s.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.f,  1.f,  1.f,  1.f);
    s.Colors[ImGuiCol_Button]                = ImVec4(.04f, .04f, .04f, 1.f);
    s.Colors[ImGuiCol_ButtonHovered]         = ImVec4(.08f, .08f, .08f, 1.f);
    s.Colors[ImGuiCol_ButtonActive]          = ImVec4(.12f, .12f, .12f, 1.f);
    s.Colors[ImGuiCol_Header]                = ImVec4(.06f, .06f, .06f, 1.f);
    s.Colors[ImGuiCol_HeaderHovered]         = ImVec4(.08f, .08f, .08f, 1.f);
    s.Colors[ImGuiCol_HeaderActive]          = ImVec4(.10f, .10f, .10f, 1.f);
    s.Colors[ImGuiCol_Separator]             = ImVec4(.20f, .20f, .20f, 1.f);
    s.Colors[ImGuiCol_ResizeGrip]            = ImVec4(.25f, .25f, .25f, 1.f);
    s.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(.25f, .25f, .25f, 1.f);
    s.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(.35f, .35f, .35f, 1.f);
    s.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(.26f, .59f, .98f, .35f);
    s.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(.05f, .05f, .05f, .65f);
    s_style_ok = true;
}

// ── Section (labelled child window, Wolsaw-style borders) ───────────────────
static bool begin_section(const char* label, ImVec2 size, bool scroll = false) {
    ImGuiWindowFlags f = scroll ? ImGuiWindowFlags_None
        : (ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (!ImGui::BeginChild(label, size, ImGuiChildFlags_Border, f)) return false;
    ImVec2 pos = ImGui::GetWindowPos(), sz = ImGui::GetWindowSize();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRect(pos, {pos.x+sz.x, pos.y+sz.y}, ImColor(0,0,0,255));
    dl->AddRect({pos.x+1,pos.y+1}, {pos.x+sz.x-1,pos.y+sz.y-1}, ImColor(40,40,40,255));
    dl->AddRect({pos.x+2,pos.y+2}, {pos.x+sz.x-2,pos.y+sz.y-2}, ImColor(25,25,25,255));
    // Label chip
    ImVec2 lsz = ImGui::CalcTextSize(label);
    ImVec2 lpos(pos.x + 15.f, pos.y - lsz.y * 0.5f);
    ImDrawList* fdl = ImGui::GetForegroundDrawList();
    fdl->AddRectFilled({lpos.x-4, lpos.y}, {lpos.x+lsz.x+4, lpos.y+lsz.y},
                       ImColor(15,15,15,255));
    fdl->AddText(lpos, ImColor(255,255,255,255), label);
    return true;
}
static void end_section() { ImGui::EndChild(); }

// ── Window chrome (multi-layer border, Wolsaw-style) ─────────────────────────
static bool begin_window(float sw, float sh) {
    const float WW = 1400.f, WH = 850.f;
    ImGui::SetNextWindowSizeConstraints({850,500}, {WW,WH});
    ImGui::SetNextWindowSize({WW,WH}, ImGuiCond_Once);
    ImGui::SetNextWindowPos({(sw-WW)*.5f, (sh-WH)*.5f}, ImGuiCond_Once);
    if (!ImGui::Begin("##kwnd", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse))
        return false;
    // Layered border
    static const struct { int off; int r,g,b,a; } borders[] = {
        {0, 0,0,0,255}, {1, 55,55,55,255}, {2, 35,35,35,255},
        {3, 35,35,35,255},{4,35,35,35,255},{5,35,35,35,255},
        {6, 35,35,35,255},{7,55,55,55,255},
    };
    ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
    for (auto& b : borders) {
        float i = (float)b.off;
        ImGui::GetWindowDrawList()->AddRect(
            {wp.x+i, wp.y+i}, {wp.x+ws.x-i, wp.y+ws.y-i},
            ImColor(b.r,b.g,b.b,b.a));
    }
    return true;
}
static void end_window() { ImGui::End(); }

// ── Half-width size for two-column sections ───────────────────────────────────
static ImVec2 half_sz() {
    const ImGuiStyle& st = ImGui::GetStyle();
    return ImVec2(
        (ImGui::GetWindowSize().x - (st.WindowPadding.x*2.f + st.ItemSpacing.x)) * .5f,
        ImGui::GetWindowSize().y - CONTENT_OFF - (st.WindowPadding.y*2.f + st.ItemSpacing.y*2.f));
}

// ── Title bar ─────────────────────────────────────────────────────────────────
static void render_title() {
    const ImGuiStyle& st = ImGui::GetStyle();
    if (!begin_section(" ",
            {ImGui::GetWindowSize().x - st.WindowPadding.x*2.f, TITLE_H})) return;
    const char* lbl = "KERNEL";
    ImVec2 ts  = ImGui::CalcTextSize(lbl);
    ImVec2 wpos = ImGui::GetWindowPos(), wsz = ImGui::GetWindowSize();
    ImVec2 lp(wpos.x + wsz.x*.5f - ts.x*.5f, wpos.y + wsz.y*.5f - ts.y*.5f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddText({lp.x+1, lp.y+1}, ImColor(0,0,0,180), lbl);
    dl->AddText(lp, ImColor(255,255,255,255), lbl);
    end_section();
}

// ── Tab bar ───────────────────────────────────────────────────────────────────
static void render_tab_bar() {
    const ImGuiStyle& st = ImGui::GetStyle();
    if (!begin_section("  ",
            {ImGui::GetWindowSize().x - st.WindowPadding.x*2.f, TABS_H})) return;
    const char* names[] = {"Visuals","Aimbot","Misc"};
    for (int i = 0; i < 3; ++i) {
        char id[32]; snprintf(id, sizeof(id), "%s##t%d", names[i], i);
        ImGui::PushStyleColor(ImGuiCol_Text,
            s_tab==i ? IM_COL32(255,255,255,255) : IM_COL32(150,150,150,255));
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, {.5f, .5f});
        float tw = (ImGui::GetWindowSize().x / 3.f) -
                   (st.FramePadding.x + st.WindowPadding.x);
        if (ImGui::Selectable(id, s_tab==i, 0, {tw, 0.f})) s_tab = i;
        ImGui::PopStyleVar(); ImGui::PopStyleColor();
        if (i != 2) ImGui::SameLine();
    }
    end_section();
}

// ── TAB 0 — VISUALS ──────────────────────────────────────────────────────────
static void tab_visuals(MenuState& m) {
    ImVec2 cs = half_sz();
    if (begin_section("esp", cs, true)) {
        ImGui::Checkbox("Box ESP",    &m.esp_box);
        ImGui::Checkbox("Health Bar", &m.esp_hp);
        ImGui::Checkbox("Head Dot",   &m.esp_head);
        ImGui::Checkbox("Name Tag",   &m.esp_name);
        ImGui::Checkbox("Distance",   &m.esp_dist);
        ImGui::Checkbox("Enemy Only", &m.enemy_only);
        ImGui::Separator();
        ImGui::TextDisabled("Max Distance (m)");
        ImGui::SetNextItemWidth(-1.f);
        ImGui::SliderFloat("##dist", &m.max_dist, 50.f, 500.f, "%.0f");
    }
    end_section();
    ImGui::SameLine();
    if (begin_section("info", cs, true)) {
        ImGui::TextDisabled("No extra visual options yet.");
    }
    end_section();
}

// ── TAB 1 — AIMBOT ───────────────────────────────────────────────────────────
static void tab_aimbot(MenuState& m) {
    ImVec2 cs = half_sz();
    if (begin_section("aimbot", cs, true)) {
        ImGui::Checkbox("Enable Aimbot", &m.aimbot);
        if (m.aimbot) {
            ImGui::Checkbox("Silent Aim",  &m.silent_aim);
            ImGui::Separator();
            ImGui::TextDisabled("FOV");
            ImGui::SetNextItemWidth(-1.f);
            ImGui::SliderFloat("##fov",    &m.aim_fov,    5.f, 180.f, "%.0f deg");
            ImGui::TextDisabled("Smoothing");
            ImGui::SetNextItemWidth(-1.f);
            ImGui::SliderFloat("##smooth", &m.aim_smooth, 0.01f, 1.f, "%.2f");
        }
    }
    end_section();
    ImGui::SameLine();
    if (begin_section("weapon", cs, true)) {
        ImGui::Checkbox("No Recoil",  &m.no_recoil);
        ImGui::Separator();
        ImGui::TextDisabled("Target Bone");
        if (ImGui::RadioButton("Head",  m.aim_bone == 0)) m.aim_bone = 0;
        ImGui::SameLine(0.f, 20.f);
        if (ImGui::RadioButton("Chest", m.aim_bone == 1)) m.aim_bone = 1;
    }
    end_section();
}

// ── TAB 2 — MISC ─────────────────────────────────────────────────────────────
static void tab_misc(MenuState& m, const char* key_display, bool driver_ok) {
    ImVec2 cs = half_sz();
    if (begin_section("movement", cs, true)) {
        ImGui::Checkbox("Speed Hack", &m.speed_hack);
        if (m.speed_hack) {
            ImGui::TextDisabled("Speed Multiplier");
            ImGui::SetNextItemWidth(-1.f);
            ImGui::SliderFloat("##spd", &m.speed_mult, 1.f, 3.f, "%.1fx");
        }
    }
    end_section();
    ImGui::SameLine();
    if (begin_section("system", cs, true)) {
        ImGui::TextDisabled("Info");
        ImGui::Text("Driver   %s", driver_ok ? "Connected" : "Offline");
        ImGui::Text("Render   OpenGL ES 2.0");
        if (key_display && key_display[0])
            ImGui::Text("Key      %.20s", key_display);
    }
    end_section();
}

// ── Public API ────────────────────────────────────────────────────────────────
bool menu_draw(MenuState& menu, ESPConfig& esp_cfg,
               const char* key_display, bool driver_ok) {
    if (!menu.visible) return true;

    ImGuiIO& io = ImGui::GetIO();
    float sw = io.DisplaySize.x, sh = io.DisplaySize.y;

    if (menu.force_center) {
        const float WW = 1400.f, WH = 850.f;
        ImGui::SetNextWindowPos({(sw-WW)*.5f, (sh-WH)*.5f}, ImGuiCond_Always);
        menu.force_center = false;
    }

    init_style();

    // Fade in
    const float dt = ImClamp(io.DeltaTime * 10.f, 0.f, 1.f);
    s_alpha = s_alpha + (1.f - s_alpha) * dt;
    ImGui::GetStyle().Alpha = s_alpha;

    // Dim background
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        {0,0}, {sw,sh}, IM_COL32(0,0,0,(int)(110*s_alpha)));

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, s_alpha);
    bool open = true;
    if (begin_window(sw, sh)) {
        render_title();
        switch (s_tab) {
        case 0: tab_visuals(menu); break;
        case 1: tab_aimbot(menu);  break;
        default: tab_misc(menu, key_display, driver_ok); break;
        }
        render_tab_bar();
        end_window();
    }
    ImGui::PopStyleVar();

    return open;
}

void menu_apply(const MenuState& menu, ESPConfig& esp) {
    esp.box_esp      = menu.esp_box;
    esp.health_bar   = menu.esp_hp;
    esp.head_dot     = menu.esp_head;
    esp.name_tag     = menu.esp_name;
    esp.distance_tag = menu.esp_dist;
    esp.show_enemies = true;
    esp.show_team    = !menu.enemy_only;
    esp.max_dist     = menu.max_dist;
}
