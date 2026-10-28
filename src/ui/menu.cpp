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
    static void apply_theme(int theme) {
        ImGuiStyle& s = ImGui::GetStyle();
        // shape — same for all themes
        s.WindowBorderSize = 1.f; s.ChildBorderSize  = 1.f;
        s.FrameBorderSize  = 1.f; s.WindowRounding   = 6.f;
        s.ChildRounding    = 4.f; s.FrameRounding    = 4.f;
        s.PopupRounding    = 4.f; s.ScrollbarRounding= 4.f;
        s.GrabRounding     = 4.f;
        s.WindowPadding = ImVec2(16.f, 16.f);
        s.FramePadding  = ImVec2(8.f,  4.f);
        s.ItemSpacing   = ImVec2(12.f, 10.f);
        s.ScrollbarSize = 10.f;

        if (theme == 1) {
            // ── Light ────────────────────────────────────────────────────────
            ImGui::StyleColorsLight();
            s.Colors[ImGuiCol_WindowBg]  = ImVec4(.94f,.94f,.94f,1.f);
            s.Colors[ImGuiCol_ChildBg]   = ImVec4(.88f,.88f,.88f,1.f);
            s.Colors[ImGuiCol_FrameBg]   = ImVec4(.80f,.80f,.80f,1.f);
            s.Colors[ImGuiCol_Text]      = ImVec4(.10f,.10f,.10f,1.f);
            s.Colors[ImGuiCol_TextDisabled]=ImVec4(.50f,.50f,.50f,1.f);
            s.Colors[ImGuiCol_Button]    = ImVec4(.75f,.75f,.75f,1.f);
            s.Colors[ImGuiCol_ButtonHovered]=ImVec4(.65f,.65f,.65f,1.f);
            s.Colors[ImGuiCol_ButtonActive] =ImVec4(.55f,.55f,.55f,1.f);
            s.Colors[ImGuiCol_Header]       =ImVec4(.78f,.78f,.78f,1.f);
            s.Colors[ImGuiCol_HeaderHovered]=ImVec4(.70f,.70f,.70f,1.f);
            s.Colors[ImGuiCol_CheckMark]    =ImVec4(.20f,.20f,.80f,1.f);
            s.Colors[ImGuiCol_SliderGrab]   =ImVec4(.20f,.20f,.80f,1.f);
            s.Colors[ImGuiCol_Border]       =ImVec4(.60f,.60f,.60f,1.f);
        } else if (theme == 2) {
            // ── Neon ─────────────────────────────────────────────────────────
            ImGui::StyleColorsDark();
            s.Colors[ImGuiCol_WindowBg]  = ImVec4(.02f,.02f,.05f,1.f);
            s.Colors[ImGuiCol_ChildBg]   = ImVec4(.03f,.03f,.08f,1.f);
            s.Colors[ImGuiCol_FrameBg]   = ImVec4(.04f,.04f,.10f,1.f);
            s.Colors[ImGuiCol_FrameBgHovered]=ImVec4(.06f,.06f,.16f,1.f);
            s.Colors[ImGuiCol_FrameBgActive] =ImVec4(.08f,.08f,.20f,1.f);
            s.Colors[ImGuiCol_Text]      = ImVec4(.0f, 1.f, .9f, 1.f);
            s.Colors[ImGuiCol_TextDisabled]=ImVec4(.0f,.5f,.45f,1.f);
            s.Colors[ImGuiCol_CheckMark] = ImVec4(.0f,1.f,.5f,1.f);
            s.Colors[ImGuiCol_SliderGrab]= ImVec4(.0f,.8f,1.f,1.f);
            s.Colors[ImGuiCol_SliderGrabActive]=ImVec4(.0f,1.f,.8f,1.f);
            s.Colors[ImGuiCol_Button]    = ImVec4(.0f,.15f,.25f,1.f);
            s.Colors[ImGuiCol_ButtonHovered]=ImVec4(.0f,.25f,.40f,1.f);
            s.Colors[ImGuiCol_ButtonActive] =ImVec4(.0f,.35f,.55f,1.f);
            s.Colors[ImGuiCol_Header]       =ImVec4(.0f,.20f,.30f,1.f);
            s.Colors[ImGuiCol_HeaderHovered]=ImVec4(.0f,.30f,.45f,1.f);
            s.Colors[ImGuiCol_Border]       =ImVec4(.0f,.5f,.8f,.6f);
            s.Colors[ImGuiCol_Separator]    =ImVec4(.0f,.4f,.6f,1.f);
            s.Colors[ImGuiCol_TitleBgActive]=ImVec4(.0f,.15f,.25f,1.f);
        } else {
            // ── Dark (default) ───────────────────────────────────────────────
            ImGui::StyleColorsDark();
            s.Colors[ImGuiCol_WindowBg]  = ImVec4(.04f,.04f,.04f,1.f);
            s.Colors[ImGuiCol_ChildBg]   = ImVec4(.06f,.06f,.06f,1.f);
            s.Colors[ImGuiCol_FrameBg]   = ImVec4(.04f,.04f,.04f,1.f);
            s.Colors[ImGuiCol_FrameBgHovered]=ImVec4(.07f,.07f,.07f,1.f);
            s.Colors[ImGuiCol_FrameBgActive] =ImVec4(.10f,.10f,.10f,1.f);
            s.Colors[ImGuiCol_Text]      = ImVec4(1.f, 1.f, 1.f, 1.f);
            s.Colors[ImGuiCol_TextDisabled]=ImVec4(.5f,.5f,.5f,1.f);
            s.Colors[ImGuiCol_CheckMark] = ImVec4(1.f,1.f,1.f,1.f);
            s.Colors[ImGuiCol_SliderGrab]= ImVec4(1.f,1.f,1.f,1.f);
            s.Colors[ImGuiCol_SliderGrabActive]=ImVec4(1.f,1.f,1.f,1.f);
            s.Colors[ImGuiCol_Button]    = ImVec4(.04f,.04f,.04f,1.f);
            s.Colors[ImGuiCol_ButtonHovered]=ImVec4(.08f,.08f,.08f,1.f);
            s.Colors[ImGuiCol_ButtonActive] =ImVec4(.12f,.12f,.12f,1.f);
            s.Colors[ImGuiCol_Header]       =ImVec4(.06f,.06f,.06f,1.f);
            s.Colors[ImGuiCol_HeaderHovered]=ImVec4(.08f,.08f,.08f,1.f);
            s.Colors[ImGuiCol_Border]       =ImVec4(0.f, 0.f, 0.f, 1.f);
            s.Colors[ImGuiCol_Separator]    =ImVec4(.20f,.20f,.20f,1.f);
        }
    }

    static void init_style() {
        if (g_style_init) return;
        apply_theme(0);
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
    // TAB 0 — VISUALS
    // ════════════════════════════════════════════════════════════════════════
    static void tab_visuals() {
        const ImVec2 cs = half_size();

        // ── Left: toggles ────────────────────────────────────────────────────
        if (begin_section("esp", cs, true)) {
            push_bold();
            ImGui::Checkbox("Enable ESP",   &Menu::cfg.espEnabled);
            sep();
            ImGui::BeginDisabled(!Menu::cfg.espEnabled);
            ImGui::Checkbox("Name",         &Menu::cfg.name);
            ImGui::Checkbox("Box",          &Menu::cfg.boxEsp);
            ImGui::Checkbox("Health bar",   &Menu::cfg.healthBar);
            ImGui::Checkbox("Snap line",    &Menu::cfg.snapline);
            ImGui::Checkbox("Enemy count",  &Menu::cfg.enemyCount);
            ImGui::Checkbox("Distance",     &Menu::cfg.distance);
            ImGui::Checkbox("Skeleton",     &Menu::cfg.skeleton);
            ImGui::EndDisabled();
            pop_bold();
        }
        end_section();
        ImGui::SameLine();

        // ── Right: colors + thickness ────────────────────────────────────────
        if (begin_section("colors & thickness", cs, true)) {
            push_bold();
            ImGui::BeginDisabled(!Menu::cfg.espEnabled);

            auto row = [](const char* lbl, ImVec4* col, float* thick, float tmin=0.5f, float tmax=5.f) {
                ImGui::TextUnformatted(lbl);
                ImGui::SetNextItemWidth(100.f);
                std::string cid = std::string("##c_") + lbl;
                ImGui::ColorEdit4(cid.c_str(), &col->x,
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                if (thick) {
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(-1.f);
                    std::string tid = std::string("##t_") + lbl;
                    ImGui::SliderFloat(tid.c_str(), thick, tmin, tmax, "%.1f px");
                }
                ImGui::Spacing();
            };

            row("Name",        &Menu::cfg.nameColor,       &Menu::cfg.nameThick);
            row("Box",         &Menu::cfg.boxColor,        &Menu::cfg.boxThick);
            row("Health bar",  &Menu::cfg.hpBarColor,      &Menu::cfg.hpBarThick);
            row("Snap line",   &Menu::cfg.snapLineColor,   &Menu::cfg.snapLineThick);
            row("Enemy count", &Menu::cfg.enemyCountColor, nullptr);
            row("Distance",    &Menu::cfg.distanceColor,   &Menu::cfg.distanceThick);
            row("Skeleton",    &Menu::cfg.skeletonColor,   &Menu::cfg.skeletonThick);

            ImGui::EndDisabled();
            pop_bold();
        }
        end_section();
    }

    // ════════════════════════════════════════════════════════════════════════
    // TAB 1 — SETTINGS
    // ════════════════════════════════════════════════════════════════════════
    static void tab_settings() {
        const ImVec2 cs = half_size();

        if (begin_section("info", cs, true)) {
            push_bold();
            ImGui::TextDisabled("Version");
            ImGui::TextUnformatted("CLAUDE PRO v1.0");
            ImGui::TextUnformatted("Developer: @PICKMEBOY00");
            sep();
            ImGui::TextDisabled("System");
            {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "Screen: %.0f x %.0f", g_sw, g_sh);
                ImGui::TextUnformatted(buf);
                std::snprintf(buf, sizeof(buf), "FPS: %.0f", ImGui::GetIO().Framerate);
                ImGui::TextUnformatted(buf);
            }
            pop_bold();
        }
        end_section();
        ImGui::SameLine();

        if (begin_section("appearance & exit", cs, true)) {
            push_bold();
            ImGui::TextDisabled("Theme");
            const char* themes[3] = {"Dark","Light","Neon"};
            for (int i = 0; i < 3; i++) {
                bool sel = (Menu::cfg.theme == i);
                if (sel) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(.25f,.45f,.75f,1.f));
                if (ImGui::Selectable(themes[i], sel, 0, ImVec2(0, 0)))
                    Menu::cfg.theme = i;
                if (sel) ImGui::PopStyleColor();
            }
            sep();
            ImGui::PushStyleColor(ImGuiCol_Button,         ImVec4(.35f,.06f,.06f,1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(.55f,.10f,.10f,1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(.70f,.14f,.14f,1.f));
            if (ImGui::Button("Exit", ImVec2(-1.f, 0.f)))
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
            case 0: tab_visuals();  break;
            case 1: tab_settings(); break;
            default: tab_visuals(); break;
        }
    }

    static void render_tab_bar() {
        const char* names[2]={"visuals","settings"};
        const ImGuiStyle& st=ImGui::GetStyle();
        if (!begin_section("  ",
            ImVec2(ImGui::GetWindowSize().x-st.WindowPadding.x*2.f,TABS_HEIGHT)))
            return;
        push_bold();
        for(int i=0;i<2;++i){
            char id[32]; std::snprintf(id,sizeof(id),"%s##t%d",names[i],i);
            ImGui::PushStyleColor(ImGuiCol_Text,
                g_tab==i?IM_COL32(255,255,255,255):IM_COL32(150,150,150,255));
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign,ImVec2(.5f,.5f));
            float tw=(ImGui::GetWindowSize().x/2.f)-(st.FramePadding.x+st.WindowPadding.x);
            if(ImGui::Selectable(id,g_tab==i,0,ImVec2(tw,0.f))) g_tab=i;
            ImGui::PopStyleVar(); ImGui::PopStyleColor();
            if(i!=1) ImGui::SameLine();
        }
        pop_bold();
        end_section();
    }

    static void render_title() {
        const ImGuiStyle& st=ImGui::GetStyle();
        if(!begin_section(" ",
            ImVec2(ImGui::GetWindowSize().x-st.WindowPadding.x*2.f,TITLE_HEIGHT)))
            return;
        const char* label="CLAUDE PRO | BY @PICKMEBOY00";
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
        apply_theme(Menu::cfg.theme);

        const float dt=ImClamp(ImGui::GetIO().DeltaTime*10.f,0.f,1.f);
        g_alpha=lerp_float(g_alpha,bar::g_open?1.f:0.f,dt);

        // Alpha = 1 чтобы ESP и ватермарк были видны при закрытом меню
        ImGui::GetStyle().Alpha=1.0f;

        // Синхронизируем Menu::cfg → cfg::* и акцент ватермарка
        SyncMenuCfg();
        SyncAccentFromMenu();


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
