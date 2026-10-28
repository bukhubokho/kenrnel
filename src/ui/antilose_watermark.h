#pragma once
#include "imgui.h"
#include <cstdio>
#include <cmath>

// DrawRadialGradientEsp and accentColor are defined in antilose_compat.cpp
extern void DrawRadialGradientEsp(const ImVec2& center, float radius, ImColor col_in);
extern ImVec4 accentColor;

// ────────────────────────────────────────────────────────────────────────────
//  NexuraWare watermark  —  zenin-style redesign
//  Layout: [N] NexuraWare | icon label | icon label | icon label | icon label
//  Icons drawn with ImDrawList primitives (no font needed)
// ────────────────────────────────────────────────────────────────────────────

namespace wm_icons {

static inline void draw_tag(ImDrawList* dl, ImVec2 pos, float sz, ImU32 col, float th) {
    const float x = pos.x, y = pos.y;
    const ImVec2 pts[] = {
        ImVec2(x + sz*.16f, y + sz*.18f), ImVec2(x + sz*.53f, y + sz*.18f),
        ImVec2(x + sz*.87f, y + sz*.52f), ImVec2(x + sz*.53f, y + sz*.87f),
        ImVec2(x + sz*.16f, y + sz*.53f), ImVec2(x + sz*.16f, y + sz*.18f)
    };
    dl->AddPolyline(pts, 6, col, ImDrawFlags_None, th);
    dl->AddCircleFilled(ImVec2(x + sz*.33f, y + sz*.33f), sz*.065f, col);
}

static inline void draw_globe(ImDrawList* dl, ImVec2 pos, float sz, ImU32 col, float th) {
    ImVec2 c(pos.x + sz*.5f, pos.y + sz*.5f);
    float  r = sz*.41f;
    dl->AddCircle(c, r, col, 32, th);
    dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x + r, c.y), col, th*.75f);
    dl->AddEllipse(c, sz*.20f, r, col, 0.f, 32, th*.75f);
    dl->AddLine(ImVec2(c.x - sz*.34f, c.y - sz*.19f), ImVec2(c.x + sz*.34f, c.y - sz*.19f), col, th*.6f);
    dl->AddLine(ImVec2(c.x - sz*.34f, c.y + sz*.19f), ImVec2(c.x + sz*.34f, c.y + sz*.19f), col, th*.6f);
}

static inline void draw_git(ImDrawList* dl, ImVec2 pos, float sz, ImU32 col, float th) {
    const float x = pos.x, y = pos.y;
    dl->AddLine(ImVec2(x + sz*.28f, y + sz*.19f), ImVec2(x + sz*.28f, y + sz*.75f), col, th);
    const ImVec2 pts[] = {
        ImVec2(x + sz*.28f, y + sz*.48f), ImVec2(x + sz*.47f, y + sz*.48f),
        ImVec2(x + sz*.70f, y + sz*.31f), ImVec2(x + sz*.70f, y + sz*.19f)
    };
    dl->AddPolyline(pts, 4, col, ImDrawFlags_None, th);
    dl->AddCircle(ImVec2(x + sz*.28f, y + sz*.19f), sz*.10f, col, 20, th);
    dl->AddCircle(ImVec2(x + sz*.70f, y + sz*.19f), sz*.10f, col, 20, th);
    dl->AddCircle(ImVec2(x + sz*.28f, y + sz*.75f), sz*.10f, col, 20, th);
}

static inline void draw_monitor(ImDrawList* dl, ImVec2 pos, float sz, ImU32 col, float th) {
    const float x = pos.x, y = pos.y;
    dl->AddRect(ImVec2(x + sz*.05f, y + sz*.08f), ImVec2(x + sz*.95f, y + sz*.70f), col, sz*.03f, 0, th);
    dl->AddLine(ImVec2(x + sz*.5f, y + sz*.70f), ImVec2(x + sz*.5f, y + sz*.84f), col, th);
    dl->AddLine(ImVec2(x + sz*.32f, y + sz*.89f), ImVec2(x + sz*.68f, y + sz*.89f), col, th);
}

} // namespace wm_icons

class Watermark {
public:
    void Render(ImFont* font = nullptr) {
        ImGuiIO& io   = ImGui::GetIO();
        int fps        = static_cast<int>(io.Framerate);

        ImDrawList* dl = ImGui::GetForegroundDrawList();

        // ── Scale to display ──────────────────────────────────────────────────
        const float sc        = (io.DisplaySize.x / 1920.f) * 1.0f;  // smaller watermark
        const float h         = 34.f * sc;
        const float rx        = 12.f * sc;
        const float ry        = 10.f * sc;
        const float pad       = 12.f * sc;
        const float text_sz   = 11.5f * sc;
        const float divider_h = 18.f * sc;
        const float icon_sz   = 13.f * sc;
        const float icon_gap  = 5.f  * sc;
        const float div_gap   = 10.f * sc;
        const float item_gap  = 5.f  * sc;
        const float logo_sz   = 20.f * sc;

        // ── Accent: taken from menu accent (set via accentColor) ──────────────
        const ImU32 col_accent = IM_COL32(
            (int)(accentColor.x * 255.f),
            (int)(accentColor.y * 255.f),
            (int)(accentColor.z * 255.f),
            255);
        const ImU32 col_blue  = IM_COL32(89, 108, 255, 255);
        const ImU32 col_gray  = IM_COL32(200, 203, 208, 255);
        const ImU32 col_text  = IM_COL32(237, 237, 238, 255);

        // ── Labels ────────────────────────────────────────────────────────────
        char fps_buf[16];
        snprintf(fps_buf, sizeof(fps_buf), "%d FPS", fps);
        const char* labels[] = { "NexuraWare", "nexuraware", fps_buf, "external" };

        // ── Measure total width ───────────────────────────────────────────────
        float content_w = logo_sz + pad + pad;  // logo + left/right padding
        ImVec2 label_sizes[4];
        for (int i = 0; i < 4; i++) {
            ImVec2 ts = ImGui::CalcTextSize(labels[i]);
            label_sizes[i] = ts;
            content_w += div_gap + icon_sz + icon_gap + ts.x + item_gap;
        }

        // ── Background ───────────────────────────────────────────────────────
        dl->AddRectFilled(
            ImVec2(rx,       ry),
            ImVec2(rx + content_w, ry + h),
            IM_COL32(4, 5, 8, 242), 14.f * sc);
        dl->AddRect(
            ImVec2(rx,       ry),
            ImVec2(rx + content_w, ry + h),
            IM_COL32(255, 255, 255, 18), 14.f * sc, 0, 1.f * sc);

        float cur_x = rx + pad;

        // ── Logo: dark circle + letter "N" ────────────────────────────────────
        {
            ImVec2 lp(cur_x, ry + (h - logo_sz) * .5f);
            ImVec2 lc(lp.x + logo_sz * .5f, lp.y + logo_sz * .5f);
            // dark backing circle (like zenin's draw_z)
            dl->AddCircleFilled(lc, logo_sz * .5f, IM_COL32(33, 14, 18, 255));
            // Letter "N" centered
            const char* mark   = "N";
            ImVec2 tsz         = ImGui::CalcTextSize(mark);
            float  text_scale  = logo_sz * .75f / ImGui::GetFontSize();
            (void)text_scale;
            ImVec2 tpos(lc.x - tsz.x * .5f, lc.y - tsz.y * .5f);
            dl->AddText(tpos, col_accent, mark);
            // slight stroke for depth
            dl->AddText(ImVec2(tpos.x + .45f, tpos.y), col_accent, mark);
            cur_x += logo_sz + pad * .5f;
        }

        // ── 4 sections: divider | icon  label ────────────────────────────────
        for (int i = 0; i < 4; i++) {
            // vertical divider
            dl->AddLine(
                ImVec2(cur_x, ry + (h - divider_h) * .5f),
                ImVec2(cur_x, ry + (h + divider_h) * .5f),
                IM_COL32(255, 255, 255, 45), 1.f * sc);
            cur_x += div_gap;

            // icon color: i0=accent, i1=blue, i2=accent, i3=gray
            const ImU32 ic_col = (i == 1) ? col_blue : (i == 3 ? col_gray : col_accent);
            ImVec2 icon_pos(cur_x, ry + (h - icon_sz) * .5f);
            const float th_ic  = 1.25f * sc;

            if (i == 0) wm_icons::draw_tag    (dl, icon_pos, icon_sz, ic_col, th_ic);
            if (i == 1) wm_icons::draw_globe   (dl, icon_pos, icon_sz, ic_col, th_ic);
            if (i == 2) wm_icons::draw_git     (dl, icon_pos, icon_sz, ic_col, th_ic);
            if (i == 3) wm_icons::draw_monitor (dl, icon_pos, icon_sz, ic_col, th_ic);
            cur_x += icon_sz + icon_gap;

            // label text
            dl->AddText(
                ImVec2(cur_x, ry + (h - label_sizes[i].y) * .5f),
                col_text, labels[i]);
            cur_x += label_sizes[i].x + item_gap;
        }
    }
};
