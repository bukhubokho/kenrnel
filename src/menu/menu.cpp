#include "menu.h"
#include "../imgui/imgui.h"
#include <cstdio>
#include <cstring>

static int s_tab = 0;

// ── colours ──────────────────────────────────────────────────────────────────
#define C_BG        IM_COL32(13,  17,  30,  252)   // deep navy
#define C_HEADER    IM_COL32(18,  24,  42,  255)   // slightly lighter navy
#define C_BORDER    IM_COL32(52,  72, 120,  180)
#define C_ACCENT    IM_COL32(56, 189, 248,  255)   // cyan-blue (tab active / section title)
#define C_ACCENT2   IM_COL32(99, 202, 253,  200)   // lighter cyan
#define C_TAB       IM_COL32(22,  30,  54,  220)
#define C_TAB_HOV   IM_COL32(35,  52,  98,  230)
#define C_TAB_ACT   IM_COL32(56, 189, 248,  255)
#define C_BTN       IM_COL32(30,  80, 160,  220)
#define C_BTN_HOV   IM_COL32(45, 110, 200,  255)
#define C_BTN_ACT   IM_COL32(70, 150, 240,  255)
#define C_FRAME     IM_COL32(22,  30,  54,  200)
#define C_SLIDER    IM_COL32(56, 189, 248,  255)
#define C_TEXT      IM_COL32(220, 230, 255,  255)
#define C_TEXT_DIM  IM_COL32(110, 130, 175,  200)
#define C_GREEN     IM_COL32( 72, 222, 120,  255)
#define C_RED       IM_COL32(240,  70,  70,  255)
#define C_TOGGLE_ON IM_COL32( 56, 189, 248,  255)
#define C_TOGGLE_OFF IM_COL32(45,  55,  85,  255)

static void push_style() {
    ImGui::PushStyleColor(ImGuiCol_WindowBg,        C_BG);
    ImGui::PushStyleColor(ImGuiCol_TitleBg,         C_HEADER);
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive,   C_HEADER);
    ImGui::PushStyleColor(ImGuiCol_Border,          C_BORDER);
    ImGui::PushStyleColor(ImGuiCol_Button,          C_BTN);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,   C_BTN_HOV);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,    C_BTN_ACT);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,         C_FRAME);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,  IM_COL32(35,50,90,220));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,      C_SLIDER);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,IM_COL32(100,210,255,255));
    ImGui::PushStyleColor(ImGuiCol_CheckMark,       C_ACCENT);
    ImGui::PushStyleColor(ImGuiCol_Header,          IM_COL32(35,60,110,200));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,   IM_COL32(50,85,155,220));
    ImGui::PushStyleColor(ImGuiCol_Tab,             C_TAB);
    ImGui::PushStyleColor(ImGuiCol_TabActive,       IM_COL32(18,24,42,255));
    ImGui::PushStyleColor(ImGuiCol_TabHovered,      C_TAB_HOV);
    ImGui::PushStyleColor(ImGuiCol_Text,            C_TEXT);
    ImGui::PushStyleColor(ImGuiCol_Separator,       IM_COL32(40,60,110,180));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,         IM_COL32(16,22,40,250));
}
static constexpr int STYLE_COLORS = 20;

// Pill-style toggle ──────────────────────────────────────────────────────────
static void toggle(const char* label, bool* v) {
    const ImVec2 sz  = {44.f, 22.f};
    const float  r   = sz.y * 0.5f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImGui::InvisibleButton(label, sz);
    if (ImGui::IsItemClicked()) *v = !*v;

    // hover glow
    ImU32 bg = *v ? C_TOGGLE_ON : C_TOGGLE_OFF;
    if (ImGui::IsItemHovered()) bg = *v ? IM_COL32(80,205,255,255) : IM_COL32(55,68,105,255);

    dl->AddRectFilled(p, {p.x+sz.x, p.y+sz.y}, bg, r);
    // knob
    float kx = *v ? p.x + sz.x - r : p.x + r;
    dl->AddCircleFilled({kx, p.y + r}, r - 3.f, IM_COL32(255,255,255,245));

    ImGui::SameLine(0.f, 8.f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.f);
    ImGui::TextUnformatted(label);
}

// Cyan section header ────────────────────────────────────────────────────────
static void section(const char* title) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, C_ACCENT);
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Separator, C_ACCENT);
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

// Thin label above a slider ──────────────────────────────────────────────────
static void slider_row(const char* id, const char* label,
                       float* v, float lo, float hi, const char* fmt) {
    ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat(id, v, lo, hi, fmt);
    ImGui::Spacing();
}

// ── main draw ────────────────────────────────────────────────────────────────
bool menu_draw(MenuState& menu, ESPConfig& esp_cfg,
               const char* key_display, bool driver_ok) {
    if (!menu.visible) return true;

    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding        = 12.f;
    st.FrameRounding         = 6.f;
    st.TabRounding           = 6.f;
    st.ItemSpacing           = {8.f, 7.f};
    st.WindowBorderSize      = 1.f;
    st.FramePadding          = {10.f, 5.f};
    st.DisplayWindowPadding  = {0.f, 0.f};
    st.DisplaySafeAreaPadding = {0.f, 0.f};

    push_style();

    ImGuiIO& io = ImGui::GetIO();
    float sw = io.DisplaySize.x, sh = io.DisplaySize.y;
    float scale = io.FontGlobalScale > 0.f ? io.FontGlobalScale : 1.f;
    float mw = 400.f * scale, mh = 540.f * scale;
    ImGui::SetNextWindowSize({mw, mh}, ImGuiCond_Once);

    if (menu.force_center) {
        float cx = (sw - mw) * 0.5f, cy = (sh - mh) * 0.5f;
        fprintf(stderr, "[MENU] force_center: disp=%.0fx%.0f → pos=%.0f,%.0f\n",
                sw, sh, cx, cy);
        ImGui::SetNextWindowPos({cx, cy}, ImGuiCond_Always);
        menu.force_center = false;
    }

    bool open = true;
    ImGui::Begin("##KernelMenu", &open,
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoTitleBar);

    // ── Header bar ──────────────────────────────────────────────────────────
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        // navy top strip
        dl->AddRectFilled(wp, {wp.x + ws.x, wp.y + 44.f * scale},
                          C_HEADER, 12.f,
                          ImDrawFlags_RoundCornersTop);
        // bottom edge accent line
        dl->AddLine({wp.x, wp.y + 44.f*scale},
                    {wp.x + ws.x, wp.y + 44.f*scale},
                    C_BORDER, 1.f);

        ImGui::SetCursorPos({12.f, 10.f * scale});
        ImGui::PushStyleColor(ImGuiCol_Text, C_ACCENT);
        ImGui::SetWindowFontScale(1.15f * scale);
        ImGui::TextUnformatted("KERNEL");
        ImGui::SetWindowFontScale(1.f);
        ImGui::PopStyleColor();

        // status pill (right side)
        const char* status_lbl = driver_ok ? "ACTIVE" : "NO DRV";
        ImU32 status_col = driver_ok ? C_GREEN : C_RED;
        float pill_x = wp.x + ws.x - 80.f * scale;
        float pill_y = wp.y + 10.f * scale;
        dl->AddRectFilled({pill_x - 4, pill_y - 2},
                          {pill_x + 68.f*scale, pill_y + 18.f*scale},
                          IM_COL32(0,0,0,80), 8.f);
        dl->AddCircleFilled({pill_x + 6.f, pill_y + 8.f}, 4.f, status_col);
        ImGui::SetCursorPos({pill_x - wp.x + 14.f, 11.f * scale});
        ImGui::PushStyleColor(ImGuiCol_Text, status_col);
        ImGui::TextUnformatted(status_lbl);
        ImGui::PopStyleColor();

        ImGui::SetCursorPosY(48.f * scale);
    }

    // ── Tab bar ──────────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_TabActive, C_HEADER);
    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_NoTooltip)) {

        // helper: tab item — returns true on click, applies text colour
        auto tab = [&](const char* lbl, int idx) {
            bool is_act = (s_tab == idx);
            if (is_act) ImGui::PushStyleColor(ImGuiCol_Text, C_ACCENT);
            else        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
            bool opened = ImGui::BeginTabItem(lbl);
            ImGui::PopStyleColor();
            if (opened) { s_tab = idx; ImGui::EndTabItem(); }
        };

        tab("Visuals",  0);
        tab("Aimbot",   1);
        tab("Misc",     2);

        ImGui::EndTabBar();
    }
    ImGui::PopStyleColor(); // TabActive override

    ImGui::Spacing();

    // ── Content ──────────────────────────────────────────────────────────────
    if (s_tab == 0) {
        // ── VISUALS ──
        section("ESP");
        toggle("Box ESP",    &menu.esp_box);
        toggle("Health Bar", &menu.esp_hp);
        toggle("Head Dot",   &menu.esp_head);
        toggle("Name Tag",   &menu.esp_name);
        toggle("Distance",   &menu.esp_dist);
        toggle("Enemy Only", &menu.enemy_only);
        ImGui::Spacing();
        slider_row("##dist", "Max Distance (m)",
                   &menu.max_dist, 50.f, 500.f, "%.0f");
    }
    else if (s_tab == 1) {
        // ── AIMBOT ──
        section("Aimbot");
        toggle("Aimbot",     &menu.aimbot);
        toggle("Silent Aim", &menu.silent_aim);
        toggle("No Recoil",  &menu.no_recoil);
        ImGui::Spacing();
        slider_row("##fov",    "FOV",       &menu.aim_fov,    5.f,  180.f, "%.0f°");
        slider_row("##smooth", "Smoothing", &menu.aim_smooth, 0.01f,  1.f, "%.2f");

        section("Target Bone");
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
        if (ImGui::RadioButton("Head",  menu.aim_bone == 0)) menu.aim_bone = 0;
        ImGui::SameLine(0.f, 20.f);
        if (ImGui::RadioButton("Chest", menu.aim_bone == 1)) menu.aim_bone = 1;
        ImGui::PopStyleColor();
    }
    else {
        // ── MISC ──
        section("Movement");
        toggle("Speed Hack", &menu.speed_hack);
        if (menu.speed_hack) {
            slider_row("##spd", "Speed Multiplier", &menu.speed_mult, 1.f, 3.f, "%.1fx");
        }

        section("Info");
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_DIM);
        ImGui::Text("Driver   %s", driver_ok ? "Connected" : "Offline");
        ImGui::Text("Render   OpenGL ES 2.0");
        if (key_display && key_display[0])
            ImGui::Text("Key      %.20s", key_display);
        ImGui::PopStyleColor();
    }

    ImGui::End();
    ImGui::PopStyleColor(STYLE_COLORS);

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
