#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.hpp"
#include "bar.hpp"
#include "cfg.hpp"
#include "theme/theme.hpp"
#include "antilose_menu.h"        // Logo, AntiloseBindFonts, SyncAccentFromMenu, SyncMenuCfg
#include "antilose_watermark.h"   // Watermark (NexuraWare)
#include "menu_inflex.h"          // Menu::cfg, Menu::open
#include "imgui.h"
#include "imgui_internal.h"
#include <vector>
#include <string>
#include <cstdio>
#include <cmath>

extern ImFont* fontBold;
extern ImFont* fontMedium;
extern ImFont* fontDesc;
extern ImFont* espFont;
extern ImGuiWindow* g_window;

namespace ui::menu {

    static bool  g_style_init  = false;
    static int   g_tab         = 0;
    static float g_alpha       = 0.f;
    static bool  g_menu_open   = false;

    static Watermark watermark;

    static const float TITLE_HEIGHT   = 52.f;
    static const float TABS_HEIGHT    = 60.f;
    static const float CONTENT_OFFSET = TITLE_HEIGHT + TABS_HEIGHT + 30.f;
    static const float WIN_WIDTH      = 1400.f;
    static const float WIN_HEIGHT     = 850.f;

    static float lerp_float(float a, float b, float t) { return a + (b - a) * t; }

    // ── Style ────────────────────────────────────────────────────────────────
    static void init_style() {
        if (g_style_init) return;
        ImGuiStyle& s = ImGui::GetStyle();
        ImGui::StyleColorsDark();
        s.WindowBorderSize  = 1.f; s.ChildBorderSize   = 1.f;
        s.FrameBorderSize   = 1.f; s.WindowRounding    = 0.f;
        s.ChildRounding     = 0.f; s.FrameRounding     = 0.f;
        s.PopupRounding     = 0.f; s.ScrollbarRounding = 0.f;
        s.GrabRounding      = 0.f;
        s.Colors[ImGuiCol_Text]                 = ImVec4(1.f,  1.f,  1.f,  1.f);
        s.Colors[ImGuiCol_TextDisabled]         = ImVec4(.5f,  .5f,  .5f,  1.f);
        s.Colors[ImGuiCol_WindowBg]             = ImVec4(.04f, .04f, .04f, 1.f);
        s.Colors[ImGuiCol_ChildBg]              = ImVec4(.06f, .06f, .06f, 1.f);
        s.Colors[ImGuiCol_PopupBg]              = ImVec4(.08f, .08f, .08f, .94f);
        s.Colors[ImGuiCol_Border]               = ImVec4(0.f,  0.f,  0.f,  1.f);
        s.Colors[ImGuiCol_BorderShadow]         = ImVec4(0.f,  0.f,  0.f,  0.f);
        s.Colors[ImGuiCol_FrameBg]              = ImVec4(.04f, .04f, .04f, 1.f);
        s.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(.07f, .07f, .07f, 1.f);
        s.Colors[ImGuiCol_FrameBgActive]        = ImVec4(.10f, .10f, .10f, 1.f);
        s.Colors[ImGuiCol_TitleBg]              = ImVec4(.04f, .04f, .04f, 1.f);
        s.Colors[ImGuiCol_TitleBgActive]        = ImVec4(.16f, .29f, .48f, 1.f);
        s.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(.02f, .02f, .02f, .53f);
        s.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(.31f, .31f, .31f, 1.f);
        s.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(.41f, .41f, .41f, 1.f);
        s.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(.51f, .51f, .51f, 1.f);
        s.Colors[ImGuiCol_CheckMark]            = ImVec4(1.f,  1.f,  1.f,  1.f);
        s.Colors[ImGuiCol_SliderGrab]           = ImVec4(1.f,  1.f,  1.f,  1.f);
        s.Colors[ImGuiCol_SliderGrabActive]     = ImVec4(1.f,  1.f,  1.f,  1.f);
        s.Colors[ImGuiCol_Button]               = ImVec4(.04f, .04f, .04f, 1.f);
        s.Colors[ImGuiCol_ButtonHovered]        = ImVec4(.08f, .08f, .08f, 1.f);
        s.Colors[ImGuiCol_ButtonActive]         = ImVec4(.12f, .12f, .12f, 1.f);
        s.Colors[ImGuiCol_Header]               = ImVec4(.06f, .06f, .06f, 1.f);
        s.Colors[ImGuiCol_HeaderHovered]        = ImVec4(.08f, .08f, .08f, 1.f);
        s.Colors[ImGuiCol_HeaderActive]         = ImVec4(.10f, .10f, .10f, 1.f);
        s.Colors[ImGuiCol_Separator]            = ImVec4(.20f, .20f, .20f, 1.f);
        s.Colors[ImGuiCol_ResizeGrip]           = ImVec4(.25f, .25f, .25f, 1.f);
        s.Colors[ImGuiCol_ResizeGripHovered]    = ImVec4(.25f, .25f, .25f, 1.f);
        s.Colors[ImGuiCol_ResizeGripActive]     = ImVec4(.35f, .35f, .35f, 1.f);
        s.Colors[ImGuiCol_TextSelectedBg]       = ImVec4(.26f, .59f, .98f, .35f);
        s.Colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(.05f, .05f, .05f, .65f);
        s.WindowPadding = ImVec2(16.f, 16.f);
        s.FramePadding  = ImVec2(8.f,  4.f);
        s.ItemSpacing   = ImVec2(12.f, 10.f);
        s.ScrollbarSize = 10.f;
        g_style_init = true;
    }

    static void push_bold() { if (fontBold) ImGui::PushFont(fontBold); }
    static void pop_bold()  { if (fontBold) ImGui::PopFont(); }

    // ── Window ───────────────────────────────────────────────────────────────
    static bool begin_window() {
        const ImVec2 wsz(WIN_WIDTH, WIN_HEIGHT);
        ImGui::SetNextWindowSizeConstraints(ImVec2(850,500), wsz);
        ImGui::SetNextWindowSize(wsz, ImGuiCond_Once);
        ImGui::SetNextWindowPos(
            ImVec2((g_sw-wsz.x)*.5f,(g_sh-wsz.y)*.5f), ImGuiCond_Once);
        if (!ImGui::Begin("##mwnd", nullptr,
            ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoCollapse))
            return false;
        g_window = ImGui::GetCurrentWindow();
        for (int i=0;i<8;++i){
            ImColor bc;
            if(i==1||i==7) bc=ImColor(55,55,55,255);
            else if(i==0)  bc=ImColor(0,0,0,255);
            else            bc=ImColor(35,35,35,255);
            ImVec2 p0(ImGui::GetWindowPos().x+(float)i, ImGui::GetWindowPos().y+(float)i);
            ImVec2 p1(ImGui::GetWindowPos().x+ImGui::GetWindowSize().x-(float)i,
                      ImGui::GetWindowPos().y+ImGui::GetWindowSize().y-(float)i);
            ImGui::GetWindowDrawList()->AddRect(p0,p1,bc);
        }
        return true;
    }
    static void end_window() { ImGui::End(); }

    // ── Section ──────────────────────────────────────────────────────────────
    static bool begin_section(const char* label, const ImVec2& size, bool scroll=false) {
        ImGuiWindowFlags f = scroll ? ImGuiWindowFlags_None
            : (ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        if (!ImGui::BeginChild(label, size, ImGuiChildFlags_Border, f)) return false;
        const ImVec2 pos=ImGui::GetWindowPos(), sz=ImGui::GetWindowSize();
        push_bold();
        const ImVec2 lsz=ImGui::CalcTextSize(label);
        pop_bold();
        const ImVec2 lpos(pos.x+15.f, pos.y-lsz.y*.5f);
        ImDrawList* dl=ImGui::GetWindowDrawList();
        dl->AddRect(pos,ImVec2(pos.x+sz.x,pos.y+sz.y),ImColor(0,0,0,255));
        dl->AddRect(ImVec2(pos.x+1,pos.y+1),ImVec2(pos.x+sz.x-1,pos.y+sz.y-1),ImColor(40,40,40,255));
        dl->AddRect(ImVec2(pos.x+2,pos.y+2),ImVec2(pos.x+sz.x-2,pos.y+sz.y-2),ImColor(25,25,25,255));
        ImGui::GetForegroundDrawList()->AddRectFilled(
            ImVec2(lpos.x-4,lpos.y),ImVec2(lpos.x+lsz.x+4,lpos.y+lsz.y),ImColor(15,15,15,255));
        if (fontBold)
            ImGui::GetForegroundDrawList()->AddText(
                fontBold,fontBold->FontSize,lpos,ImColor(255,255,255,255),label);
        else
            ImGui::GetForegroundDrawList()->AddText(lpos,ImColor(255,255,255,255),label);
        return true;
    }
    static void end_section() { ImGui::EndChild(); }
    static void sep() { ImGui::Separator(); }

    static ImVec2 half_size() {
        const ImGuiStyle& st=ImGui::GetStyle();
        return ImVec2(
            (ImGui::GetWindowSize().x-(st.WindowPadding.x*2.f+st.ItemSpacing.x))*.5f,
            ImGui::GetWindowSize().y-CONTENT_OFFSET-(st.WindowPadding.y*2.f+st.ItemSpacing.y*2.f));
    }

    static bool combo_vec(const char* lbl, int* cur, const std::vector<const char*>& items) {
        const char* prev=(*cur>=0&&*cur<(int)items.size())?items[*cur]:"";
        bool ch=false;
        if(ImGui::BeginCombo(lbl,prev)){
            for(int i=0;i<(int)items.size();++i){
                bool sel=(*cur==i);
                if(ImGui::Selectable(items[i],sel)){*cur=i;ch=true;}
                if(sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        return ch;
    }
    static void color_ctrl(const char* lbl, ImVec4* v) {
        ImGui::TextUnformatted(lbl);
        ImGui::SetNextItemWidth(-1.f);
        std::string id=std::string("##cc_")+lbl;
        ImGui::ColorEdit4(id.c_str(),&v->x,
            ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_AlphaBar|
            ImGuiColorEditFlags_AlphaPreviewHalf);
    }

    // ════════════════════════════════════════════════════════════════════════
    // TAB 0 — AIMBOT   (все переменные → Menu::cfg.*)
    // ════════════════════════════════════════════════════════════════════════
    static void tab_aimbot() {
        const ImVec2 cs=half_size();

        if (begin_section("aimbot",cs,true)) {
            push_bold();
            ImGui::Checkbox("Enable Aimbot",   &Menu::cfg.aimbot);
            if (Menu::cfg.aimbot) {
                combo_vec("Target bone",&Menu::cfg.targetBone,
                    {"Head","Neck","Chest","Stomach","Pelvis"});
                combo_vec("Target mode",&Menu::cfg.targetMode,
                    {"Closest FOV","Low HP","Distance"});
                ImGui::Checkbox("Visible check",  &Menu::cfg.visibleCheck);
                ImGui::Checkbox("Hard lock",      &Menu::cfg.hardLock);
                ImGui::Checkbox("Silent aim",     &Menu::cfg.silentAim);
                sep();
                ImGui::SliderInt("FOV (deg)",     &Menu::cfg.fov,        10, 360);
                ImGui::SliderInt("Smooth",        &Menu::cfg.smooth,      1,  20);
                ImGui::SliderInt("Max distance",  &Menu::cfg.aimMaxDist, 10, 500);
                sep();
                ImGui::Checkbox("Draw FOV circle",&Menu::cfg.drawFov);
            }
            pop_bold();
        }
        end_section();
        ImGui::SameLine();

        if (begin_section("triggerbot & weapon",cs,true)) {
            push_bold();
            ImGui::Checkbox("Enable Triggerbot",  &Menu::cfg.triggerbot);
            if (Menu::cfg.triggerbot) {
                ImGui::Checkbox("Visible only",   &Menu::cfg.tbVisibleCheck);
                ImGui::Checkbox("Only ADS",       &Menu::cfg.tbOnlyADS);
                ImGui::Checkbox("Tap fire",       &Menu::cfg.tbTapFire);
                sep();
                ImGui::SliderInt("Delay (ms)",    &Menu::cfg.tbDelay,     0, 500);
                ImGui::SliderInt("Interval (ms)", &Menu::cfg.tbInterval,  50, 1000);
                ImGui::SliderInt("Fire X %",      &Menu::cfg.tbFireXPct,  50, 100);
                ImGui::SliderInt("Fire Y %",      &Menu::cfg.tbFireYPct,  50, 100);
            }
            sep();
            ImGui::TextDisabled("Weapon");
            ImGui::Checkbox("No Recoil",  &Menu::cfg.noRecoil);
            ImGui::Checkbox("No Spread",  &Menu::cfg.noSpread);
            ImGui::Checkbox("Rapid Fire", &Menu::cfg.rapidFire);
            pop_bold();
        }
        end_section();
    }

    // ════════════════════════════════════════════════════════════════════════
    // TAB 1 — VISUALS
    // ════════════════════════════════════════════════════════════════════════
    static void tab_visuals() {
        const ImVec2 cs=half_size();

        if (begin_section("esp",cs,true)) {
            push_bold();
            ImGui::Checkbox("Box ESP",    &Menu::cfg.boxEsp);
            ImGui::Checkbox("Name",       &Menu::cfg.name);
            ImGui::Checkbox("Health bar", &Menu::cfg.healthBar);
            ImGui::Checkbox("Distance",   &Menu::cfg.distance);
            ImGui::Checkbox("Snapline",   &Menu::cfg.snapline);
            ImGui::Checkbox("Head dot",   &Menu::cfg.headdot);
            ImGui::Checkbox("Skeleton",   &Menu::cfg.skeleton);
            sep();
            combo_vec("Box style",&Menu::cfg.boxStyle,{"2D","Corner","3D"});
            ImGui::SliderInt("Max dist",  &Menu::cfg.espDist, 50, 600);
            pop_bold();
        }
        end_section();
        ImGui::SameLine();

        if (begin_section("colors & effects",cs,true)) {
            push_bold();
            ImGui::TextDisabled("ESP Glow");
            ImGui::Checkbox("Enable glow",    &Menu::cfg.espGlow);
            if (Menu::cfg.espGlow) {
                ImGui::SliderInt("Glow size",  &Menu::cfg.espGlowSize,  4, 24);
                ImGui::SliderInt("Glow alpha", &Menu::cfg.espGlowAlpha, 5, 80);
            }
            sep();
            ImGui::TextDisabled("Effects");
            ImGui::Checkbox("Hit particles", &Menu::cfg.hitParticles);
            ImGui::Checkbox("Death effect",  &Menu::cfg.deathEffect);
            ImGui::Checkbox("Death ring",    &Menu::cfg.deathRing);
            pop_bold();
        }
        end_section();
    }

    // ════════════════════════════════════════════════════════════════════════
    // TAB 2 — WORLD
    // ════════════════════════════════════════════════════════════════════════
    static void tab_world() {
        const ImVec2 cs=half_size();

        if (begin_section("world & render",cs,true)) {
            push_bold();
            ImGui::Checkbox("Potato GFX",  &Menu::cfg.potatoGfx);
            ImGui::Checkbox("Full bright", &Menu::cfg.fullBright);
            ImGui::Checkbox("Night mode",  &Menu::cfg.nightMode);
            sep();
            ImGui::SliderInt("Time of day",&Menu::cfg.timeOfDay, 0, 24);
            sep();
            ImGui::TextDisabled("Ambient Light");
            ImGui::Checkbox("Enable##amb", &Menu::cfg.ambientLight);
            if (Menu::cfg.ambientLight) {
                color_ctrl("Ambient color",&Menu::cfg.ambientColor);
                ImGui::SliderInt("Intensity",    &Menu::cfg.ambientIntensity, 0, 85);
                ImGui::Checkbox("Pulse",         &Menu::cfg.ambientPulse);
                if (Menu::cfg.ambientPulse)
                    ImGui::SliderInt("Pulse speed",&Menu::cfg.ambientSpeed, 5, 50);
            }
            pop_bold();
        }
        end_section();
        ImGui::SameLine();

        if (begin_section("bullet tracers",cs,true)) {
            push_bold();
            ImGui::Checkbox("Enable##tr",    &Menu::cfg.tracerEnabled);
            if (Menu::cfg.tracerEnabled) {
                combo_vec("Mode",&Menu::cfg.tracerMode,{"2D","3D"});
                ImGui::SliderInt("Thickness", &Menu::cfg.tracerThickness, 5, 40);
                ImGui::SliderInt("Lifetime",  &Menu::cfg.tracerLifetime,  3, 30);
                ImGui::Checkbox("Spark",      &Menu::cfg.tracerSpark);
                ImGui::Checkbox("Fade",       &Menu::cfg.tracerFade);
            }
            pop_bold();
        }
        end_section();
    }

    // ════════════════════════════════════════════════════════════════════════
    // TAB 3 — RADAR & MISC
    // ════════════════════════════════════════════════════════════════════════
    static void tab_radar_misc() {
        const ImVec2 cs=half_size();

        if (begin_section("mini radar",cs,true)) {
            push_bold();
            ImGui::Checkbox("Enable radar",  &Menu::cfg.miniRadar);
            ImGui::BeginDisabled(!Menu::cfg.miniRadar);
            combo_vec("Style",&Menu::cfg.radarStyle,{"Round","Square"});
            ImGui::SliderInt("Size",          &Menu::cfg.radarSize,  80, 300);
            ImGui::SliderInt("Range",         &Menu::cfg.radarRange, 20, 200);
            ImGui::Checkbox("Color by dist",  &Menu::cfg.radarShowDist);
            ImGui::EndDisabled();
            pop_bold();
        }
        end_section();
        ImGui::SameLine();

        if (begin_section("misc",cs,true)) {
            push_bold();
            ImGui::TextDisabled("Overlay");
            ImGui::Checkbox("Watermark",    &Menu::cfg.watermark);
            ImGui::Checkbox("FPS counter",  &Menu::cfg.fpsCounter);
            ImGui::Checkbox("Stream proof", &Menu::cfg.streamproof);
            combo_vec("FPS limit",&Menu::cfg.fpsLimit,{"30","60","90","120","144"});
            sep();
            ImGui::TextDisabled("Crosshair");
            ImGui::Checkbox("Enable##ch",   &Menu::cfg.crosshair);
            if (Menu::cfg.crosshair) {
                combo_vec("Style##ch",&Menu::cfg.crosshairStyle,
                    {"Cross","Dot","Circle","Cross+Circle"});
                ImGui::SliderInt("Size##ch",  &Menu::cfg.crosshairSize,  2, 30);
                ImGui::SliderInt("Gap##ch",   &Menu::cfg.crosshairGap,   0, 20);
                ImGui::SliderInt("Thick##ch", &Menu::cfg.crosshairThick, 5, 40);
            }
            sep();
            ImGui::TextDisabled("Hit marker");
            ImGui::Checkbox("Enable##hm",   &Menu::cfg.hitMarker);
            if (Menu::cfg.hitMarker)
                ImGui::SliderInt("HM size",  &Menu::cfg.hitMarkerSize, 4, 30);
            sep();
            ImGui::TextDisabled("Rain");
            ImGui::Checkbox("Enable##rain", &Menu::cfg.rain);
            if (Menu::cfg.rain)
                ImGui::SliderInt("Opacity",  &Menu::cfg.rainOpacity, 10, 100);
            sep();
            ImGui::TextDisabled("Speedhack");
            ImGui::Checkbox("Enable##spd",  &Menu::cfg.speedhack);
            if (Menu::cfg.speedhack)
                ImGui::SliderInt("Multiplier",&Menu::cfg.speedMult, 10, 30);
            pop_bold();
        }
        end_section();
    }

    // ════════════════════════════════════════════════════════════════════════
    // TAB 4 — SETTINGS
    // ════════════════════════════════════════════════════════════════════════
    static void tab_settings() {
        const ImVec2 cs=half_size();

        if (begin_section("hit sounds & item esp",cs,true)) {
            push_bold();
            ImGui::TextDisabled("Hit sounds");
            ImGui::Checkbox("Hit sound##hs",  &Menu::cfg.hitSound);
            if (Menu::cfg.hitSound)
                combo_vec("Sound type",&Menu::cfg.hitSoundType,{"Click","Bell","Pop"});
            ImGui::Checkbox("Hit logs",       &Menu::cfg.hitLogs);
            sep();
            ImGui::TextDisabled("Item ESP");
            ImGui::Checkbox("Enable##ie",     &Menu::cfg.itemEsp);
            if (Menu::cfg.itemEsp) {
                ImGui::Checkbox("Weapons##ie",&Menu::cfg.weapons);
                ImGui::Checkbox("Ammo##ie",  &Menu::cfg.ammo);
                ImGui::Checkbox("Medkits##ie",&Menu::cfg.medkits);
                ImGui::SliderInt("Item dist", &Menu::cfg.itemDistance, 20, 500);
            }
            sep();
            ImGui::TextDisabled("Throwables");
            ImGui::Checkbox("Grenade warn",   &Menu::cfg.grenadeWarning);
            ImGui::Checkbox("Trajectory",     &Menu::cfg.trajectory);
            if (Menu::cfg.trajectory)
                combo_vec("Prediction",&Menu::cfg.prediction,{"Low","Med","High"});
            pop_bold();
        }
        end_section();
        ImGui::SameLine();

        if (begin_section("system",cs,true)) {
            push_bold();
            ImGui::TextDisabled("Debug");
            ImGui::Checkbox("Bone size debug",&Menu::cfg.boneSize);
            ImGui::Checkbox("Auto win",       &Menu::cfg.autoWin);
            sep();
            ImGui::TextDisabled("Info");
            {
                char buf[64];
                std::snprintf(buf,sizeof(buf),"Screen: %.0f x %.0f",g_sw,g_sh);
                ImGui::TextUnformatted(buf);
                std::snprintf(buf,sizeof(buf),"FPS: %.0f",ImGui::GetIO().Framerate);
                ImGui::TextUnformatted(buf);
            }
            sep();
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(.35f,.06f,.06f,1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.55f,.10f,.10f,1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(.70f,.14f,.14f,1.f));
            if (ImGui::Button("Exit",ImVec2(-1.f,0.f)))
                Menu::cfg.exitRequested = true;
            ImGui::PopStyleColor(3);
            pop_bold();
        }
        end_section();
    }

    // ════════════════════════════════════════════════════════════════════════
    // Dispatch / Tab bar / Title
    // ════════════════════════════════════════════════════════════════════════
    static void render_tab() {
        switch(g_tab){
            case 0: tab_aimbot();     break;
            case 1: tab_visuals();    break;
            case 2: tab_world();      break;
            case 3: tab_radar_misc(); break;
            case 4: tab_settings();   break;
            default: tab_visuals();   break;
        }
    }

    static void render_tab_bar() {
        const char* names[5]={"aimbot","visuals","world","radar & misc","settings"};
        const ImGuiStyle& st=ImGui::GetStyle();
        if (!begin_section("  ",
            ImVec2(ImGui::GetWindowSize().x-st.WindowPadding.x*2.f,TABS_HEIGHT)))
            return;
        push_bold();
        for(int i=0;i<5;++i){
            char id[32]; std::snprintf(id,sizeof(id),"%s##t%d",names[i],i);
            ImGui::PushStyleColor(ImGuiCol_Text,
                g_tab==i?IM_COL32(255,255,255,255):IM_COL32(150,150,150,255));
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign,ImVec2(.5f,.5f));
            float tw=(ImGui::GetWindowSize().x/5.f)-(st.FramePadding.x+st.WindowPadding.x);
            if(ImGui::Selectable(id,g_tab==i,0,ImVec2(tw,0.f))) g_tab=i;
            ImGui::PopStyleVar(); ImGui::PopStyleColor();
            if(i!=4) ImGui::SameLine();
        }
        pop_bold();
        end_section();
    }

    static void render_title() {
        const ImGuiStyle& st=ImGui::GetStyle();
        if(!begin_section(" ",
            ImVec2(ImGui::GetWindowSize().x-st.WindowPadding.x*2.f,TITLE_HEIGHT)))
            return;
        const char* label="NexuraWare | By Wolsaw";
        push_bold();
        const ImVec2 ts=ImGui::CalcTextSize(label);
        pop_bold();
        ImVec2 lpos(
            ImGui::GetWindowPos().x+ImGui::GetWindowSize().x*.5f-ts.x*.5f,
            ImGui::GetWindowPos().y+ImGui::GetWindowSize().y*.5f-ts.y*.5f);
        ImDrawList* dl=ImGui::GetWindowDrawList();
        if(fontBold){
            dl->AddText(fontBold,fontBold->FontSize,
                ImVec2(lpos.x+1,lpos.y+1),ImColor(0,0,0,(int)(180*g_alpha)),label);
            dl->AddText(fontBold,fontBold->FontSize,
                lpos,ImColor(255,255,255,(int)(255*g_alpha)),label);
        } else {
            dl->AddText(ImVec2(lpos.x+1,lpos.y+1),ImColor(0,0,0,(int)(180*g_alpha)),label);
            dl->AddText(lpos,ImColor(255,255,255,(int)(255*g_alpha)),label);
        }
        end_section();
    }

    // ════════════════════════════════════════════════════════════════════════
    // Public API
    // ════════════════════════════════════════════════════════════════════════
    bool should_exit()           { return Menu::cfg.exitRequested; }
    void set_menu_open(bool open){ g_menu_open=open; ui::bar::g_open=open; }
    bool is_menu_open()          { return g_menu_open; }

    void render() {
        if (Menu::cfg.exitRequested) return;

        bar::render();

        // Биндим шрифты и синхронизируем Menu::open
        AntiloseBindFonts();
        Menu::open = bar::g_open;

        init_style();

        const float dt=ImClamp(ImGui::GetIO().DeltaTime*10.f,0.f,1.f);
        g_alpha=lerp_float(g_alpha,bar::g_open?1.f:0.f,dt);

        // Alpha = 1 чтобы ESP и ватермарк были видны при закрытом меню
        ImGui::GetStyle().Alpha=1.0f;

        // Синхронизируем Menu::cfg → cfg::* и акцент ватермарка
        SyncMenuCfg();
        SyncAccentFromMenu();

        // NexuraWare ватермарк — всегда
        watermark.Render(Logo);

        if(g_alpha<=0.01f) return;

        ImGui::GetBackgroundDrawList()->AddRectFilled(
            ImVec2(0,0),ImVec2(g_sw,g_sh),
            IM_COL32(0,0,0,(int)(110*g_alpha)));

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha,g_alpha);
        if(begin_window()){
            render_title();
            render_tab();
            render_tab_bar();
            end_window();
        }
        ImGui::PopStyleVar();
    }

} // namespace ui::menu
