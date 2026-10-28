// box_shadow — soft shadow for ImGui, public-API only.
// Draws nested translucent rects to fake a gaussian blur. No logic outside drawing.
#pragma once

#include "imgui.h"
#include "imgui_internal.h"

struct RectangleShadowSettings {
    ImVec2  rectPos              = ImVec2(0, 0);
    ImVec2  rectSize             = ImVec2(0, 0);
    float   sigma                = 4.0f;
    int     rings                = 4;
    float   spacingBetweenRings  = 6.0f;
    int     samplesPerCornerSide = 2;
    float   spacingBetweenSamples= 4.0f;
    ImColor shadowColor          = ImColor(0.0f, 0.0f, 0.0f, 1.0f);
    ImVec2  shadowSize           = ImVec2(0, 0);
    float   rounding             = 0.0f;
};

static inline void drawRectangleShadowVerticesAdaptive(const RectangleShadowSettings& s, bool inner = false) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (!dl) return;

    const int   rings     = s.rings > 0 ? s.rings : 1;
    const float baseAlpha = s.shadowColor.Value.w;
    if (baseAlpha <= 0.0f) return;

    const ImVec2 pmin = s.rectPos;
    const ImVec2 pmax = ImVec2(s.rectPos.x + s.rectSize.x, s.rectPos.y + s.rectSize.y);
    const float  step = s.spacingBetweenRings > 0.0f ? s.spacingBetweenRings : 4.0f;

    for (int i = rings; i >= 1; --i) {
        const float t    = (float)i / (float)rings;
        const float grow = (s.sigma + step) * t + s.shadowSize.x;
        float a = baseAlpha * (1.0f - t) / (float)rings * 1.6f;
        if (a <= 0.001f) continue;

        const ImU32 col = ImColor(s.shadowColor.Value.x, s.shadowColor.Value.y,
                                  s.shadowColor.Value.z, a);

        if (!inner) {
            dl->AddRectFilled(ImVec2(pmin.x - grow, pmin.y - grow),
                              ImVec2(pmax.x + grow, pmax.y + grow),
                              col, s.rounding + grow);
        } else {
            const float g = grow * 0.5f;
            dl->AddRect(ImVec2(pmin.x + g, pmin.y + g),
                        ImVec2(pmax.x - g, pmax.y - g),
                        col, s.rounding, 0, 1.5f);
        }
    }
}
