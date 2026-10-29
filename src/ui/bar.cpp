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

        // ── Floating circular toggle button ───────────────────────────────────
        const float   R    = 28.f;              // radius
        const float   D    = R * 2.f;
        static ImVec2 fpos = {g_sw - D - 20.f, g_sh * 0.5f - R};  // initial pos
        static bool   dragging = false;

        // Clamp to screen
        fpos.x = ImClamp(fpos.x, 0.f, g_sw - D);
        fpos.y = ImClamp(fpos.y, 0.f, g_sh - D);

        ImGui::SetNextWindowPos(fpos, ImGuiCond_Always);
        ImGui::SetNextWindowSize({D, D});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   R);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    {0.f, 0.f});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::Begin("##floatbtn", nullptr,
            ImGuiWindowFlags_NoTitleBar     | ImGuiWindowFlags_NoDecoration  |
            ImGuiWindowFlags_NoScrollbar    | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoMove         | ImGuiWindowFlags_NoResize       |
            ImGuiWindowFlags_NoBackground   | ImGuiWindowFlags_NoBringToDisplayFrontOnFocus);

        ImGui::InvisibleButton("##fbd", {D, D});

        // Drag vs tap: only toggle if mouse moved < 6px total
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0, 2.f)) {
            fpos.x += io.MouseDelta.x;
            fpos.y += io.MouseDelta.y;
            dragging = true;
        }
        if (ImGui::IsItemDeactivated()) {
            if (!dragging) g_open = !g_open;
            dragging = false;
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        // ── Draw the circle on foreground (always on top) ────────────────────
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImVec2 center  = {fpos.x + R, fpos.y + R};

        // Shadow
        dl->AddCircleFilled(center, R + 2.f, IM_COL32(0, 0, 0, 80));

        // Background circle
        ImU32 bg = g_open
            ? IM_COL32(180, 30, 30, 230)    // red when menu open
            : IM_COL32(20,  20, 20, 220);   // dark when closed
        dl->AddCircleFilled(center, R, bg);

        // Border ring
        ImU32 ring = g_open
            ? IM_COL32(255, 80,  80,  200)
            : IM_COL32(255, 255, 255, 120);
        dl->AddCircle(center, R, ring, 40, 1.8f);

        float s = R * 0.38f;
        if (g_open) {
            // X icon
            dl->AddLine({center.x-s, center.y-s}, {center.x+s, center.y+s},
                        IM_COL32(255,255,255,255), 2.5f);
            dl->AddLine({center.x+s, center.y-s}, {center.x-s, center.y+s},
                        IM_COL32(255,255,255,255), 2.5f);
        } else {
            // Hamburger (3 lines)
            float gap = s * 0.55f;
            for (int i = -1; i <= 1; i++) {
                float ly = center.y + i * gap;
                dl->AddLine({center.x - s, ly}, {center.x + s, ly},
                            IM_COL32(255,255,255,220), 2.f);
            }
        }
    }
}
