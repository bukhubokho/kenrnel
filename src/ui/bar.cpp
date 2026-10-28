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
        static float prog = 0.f;

        // g_open is now controlled externally (volume key double-press)
        // Animate prog toward the target set by g_open
        float tgt = g_open ? 1.f : 0.f;
        float spd = 12.f * io.DeltaTime;
        if (prog < tgt) prog = ImMin(prog + spd, tgt);
        else if (prog > tgt) prog = ImMax(prog - spd, tgt);
        prog = ImClamp(prog, 0.f, 1.f);

        g_alpha = ease(prog);

    }
}
