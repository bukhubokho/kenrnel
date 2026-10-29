#include "bar.hpp"
#include "theme/theme.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <cmath>

namespace ui::bar {
    static float lrp(float a, float b, float t) { return a + (b - a) * t; }
    static float ease(float t) { return -(cosf(3.14159265f * t) - 1.f) / 2.f; }

    void set_game_alpha(float a) { g_game_alpha = a; }
    float game_alpha() { return g_game_alpha; }

    void render() {
        if (g_game_alpha < 0.001f) return;

        ImGuiIO& io = ImGui::GetIO();

        // ── Animate menu alpha ────────────────────────────────────────────────
        static float prog = 0.f;
        float tgt = g_open ? 1.f : 0.f;
        float spd = 12.f * io.DeltaTime;
        if (prog < tgt) prog = ImMin(prog + spd, tgt);
        else if (prog > tgt) prog = ImMax(prog - spd, tgt);
        prog = ImClamp(prog, 0.f, 1.f);
        g_alpha = ease(prog);

        // ── Floating circle button — only visible when menu is CLOSED ─────────
        const float   R    = 52.f;
        const float   D    = R * 2.f;
        static ImVec2 fpos = {g_sw - D - 20.f, g_sh * 0.5f - R};
        static bool   dragging = false;

        fpos.x = ImClamp(fpos.x, 0.f, g_sw - D);
        fpos.y = ImClamp(fpos.y, 0.f, g_sh - D);

        if (!g_open) {
            ImGui::SetNextWindowPos(fpos, ImGuiCond_Always);
            ImGui::SetNextWindowSize({D, D});
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   R);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    {0.f, 0.f});
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f,0.f,0.f,0.f));
            ImGui::Begin("##floatbtn", nullptr,
                ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoDecoration  |
                ImGuiWindowFlags_NoScrollbar  | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoMove       | ImGuiWindowFlags_NoResize       |
                ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);

            ImGui::InvisibleButton("##fbd", {D, D});
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0, 2.f)) {
                fpos.x += io.MouseDelta.x;
                fpos.y += io.MouseDelta.y;
                dragging = true;
            }
            if (ImGui::IsItemDeactivated()) {
                if (!dragging) g_open = true;  // tap = open menu
                dragging = false;
            }

            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(3);

            // Draw circle
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            ImVec2 center  = {fpos.x + R, fpos.y + R};
            dl->AddCircleFilled(center, R + 2.f, IM_COL32(0, 0, 0, 80));        // shadow
            dl->AddCircleFilled(center, R,       IM_COL32(20, 20, 20, 225));     // bg
            dl->AddCircle(center, R, IM_COL32(255, 255, 255, 130), 40, 1.8f);   // ring
            // Hamburger icon
            float s   = R * 0.38f;
            float gap = s * 0.55f;
            for (int i = -1; i <= 1; i++) {
                float ly = center.y + i * gap;
                dl->AddLine({center.x - s, ly}, {center.x + s, ly},
                            IM_COL32(255,255,255,230), 2.2f);
            }
        }
    }
}
