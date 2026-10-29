// language: C++17, target: Android arm64
// Draws ESP overlays: 2D box, name, HP bar, snap line, distance.
// All drawing via ImDrawList — no OpenGL calls.
#include "esp_draw.hpp"
#include "Offsets.h"
#include "../driver/driver.h"
#include <cmath>
#include <cstdio>

namespace esp {

// ── Camera matrix (column-major 4x4 view-projection) ─────────────────────────
static float g_mvp[16] = {};
static bool  g_cam_ok  = false;

bool update_camera(int drv_fd, uint32_t pid, uint64_t base) {
    using namespace Offsets;
    g_cam_ok = false;
    if (drv_fd < 0 || pid == 0 || base == 0) return false;

    uint64_t gf_static = rt_read_t<uint64_t>(drv_fd, pid, base + GameFacade);
    if (!gf_static) return false;
    uint64_t base_game = rt_read_t<uint64_t>(drv_fd, pid, gf_static + GameFacade_P2);
    if (!base_game) return false;
    uint64_t match = rt_read_t<uint64_t>(drv_fd, pid, base_game + BaseGame_Match);
    if (!match) return false;
    uint64_t local = rt_read_t<uint64_t>(drv_fd, pid, match + Match_LocalPlayer);
    if (!local) return false;

    // Camera chain: local_player + Player_Camera → cam_obj + Camera_Follow → follow + Camera_IntPtr → native + Camera_Matrix
    uint64_t cam_obj = rt_read_t<uint64_t>(drv_fd, pid, local + Player_Camera);
    if (!cam_obj) return false;
    uint64_t follow  = rt_read_t<uint64_t>(drv_fd, pid, cam_obj + Camera_Follow);
    if (!follow) return false;
    uint64_t native  = rt_read_t<uint64_t>(drv_fd, pid, follow + Camera_IntPtr);
    if (!native) return false;

    rt_read(drv_fd, pid, native + Camera_Matrix, g_mvp, sizeof(g_mvp));
    g_cam_ok = true;
    return true;
}

// ── W2S ──────────────────────────────────────────────────────────────────────
static bool w2s(const ff::Vec3& world, float sw, float sh, float& sx, float& sy) {
    // column-major MVP: clip = M * [x,y,z,1]^T
    float cx = g_mvp[0]*world.x + g_mvp[4]*world.y + g_mvp[8]*world.z  + g_mvp[12];
    float cy = g_mvp[1]*world.x + g_mvp[5]*world.y + g_mvp[9]*world.z  + g_mvp[13];
    float cw = g_mvp[3]*world.x + g_mvp[7]*world.y + g_mvp[11]*world.z + g_mvp[15];
    if (cw < 0.1f) return false;
    sx = (1.f + cx / cw) * 0.5f * sw;
    sy = (1.f - cy / cw) * 0.5f * sh;
    return true;
}

static float distance(const ff::Vec3& a, const ff::Vec3& b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

static ImU32 to_col(const ImVec4& c, float alpha = 1.f) {
    return IM_COL32(
        (int)(c.x * 255), (int)(c.y * 255),
        (int)(c.z * 255), (int)(c.w * alpha * 255));
}

// ── Draw ─────────────────────────────────────────────────────────────────────
void draw(ImDrawList* dl, const ff::PlayerInfo* players, int count,
          const Menu::Settings& cfg, float sw, float sh) {
    if (!g_cam_ok || !cfg.espEnabled) return;

    // Fake local head — used for distance (camera position approximated)
    // We'll compute distance from foot position z difference
    for (int i = 0; i < count; i++) {
        const ff::PlayerInfo& p = players[i];
        if (!p.valid || p.is_local) continue;

        float hx, hy, fx, fy;
        if (!w2s(p.head_pos, sw, sh, hx, hy)) continue;
        if (!w2s(p.foot_pos, sw, sh, fx, fy)) continue;

        // Sanity: must be on screen
        if (hx < -sw || hx > 2*sw || hy < -sh || hy > 2*sh) continue;
        if (fy < hy) continue;  // head below foot = bad projection

        float box_h = fy - hy;
        float box_w = box_h * 0.4f;
        float cx    = (hx + fx) * 0.5f;

        float x1 = cx - box_w * 0.5f;
        float x2 = cx + box_w * 0.5f;
        float y1 = hy;
        float y2 = fy;

        // ── Box ──────────────────────────────────────────────────────────────
        if (cfg.boxEsp) {
            ImU32 col    = to_col(cfg.boxColor);
            ImU32 shadow = IM_COL32(0,0,0,120);
            dl->AddRect({x1+1,y1+1},{x2+1,y2+1}, shadow, 0, 0, cfg.boxThick+1.f);
            dl->AddRect({x1,y1},{x2,y2}, col, 0, 0, cfg.boxThick);
        }

        // ── Name ─────────────────────────────────────────────────────────────
        if (cfg.name && p.name[0]) {
            ImU32 ncol = to_col(cfg.nameColor);
            dl->AddText({cx - ImGui::CalcTextSize(p.name).x * 0.5f, y1 - 14.f},
                        ncol, p.name);
        }

        // ── HP bar (left of box, vertical) ───────────────────────────────────
        if (cfg.healthBar) {
            float hp_ratio = p.hp / 100.f;
            if (hp_ratio > 1.f) hp_ratio = 1.f;
            float bx  = x1 - 5.f;
            float bh  = box_h * hp_ratio;
            // background
            dl->AddRectFilled({bx-2.f, y1}, {bx, y2}, IM_COL32(0,0,0,160));
            // bar
            ImVec4 hc = {0.1f + (1.f-hp_ratio)*0.9f, hp_ratio*0.9f, 0.1f, 1.f};
            dl->AddRectFilled({bx-2.f, y2-bh}, {bx, y2}, to_col(hc));
        }

        // ── Snap line ─────────────────────────────────────────────────────────
        if (cfg.snapline) {
            dl->AddLine({sw*0.5f, 0.f}, {hx, hy},
                        to_col(cfg.snapLineColor), cfg.snapLineThick);
        }

        // ── Distance ─────────────────────────────────────────────────────────
        if (cfg.distance) {
            float dist = sqrtf(p.foot_pos.x*p.foot_pos.x +
                               p.foot_pos.y*p.foot_pos.y +
                               p.foot_pos.z*p.foot_pos.z);
            char dbuf[16];
            snprintf(dbuf, sizeof(dbuf), "%.0fm", dist);
            dl->AddText({cx - ImGui::CalcTextSize(dbuf).x*0.5f, y2+2.f},
                        to_col(cfg.distanceColor), dbuf);
        }

        // ── Enemy count dot (drawn in caller loop — skip per-player) ─────────
    }

    // ── Enemy count ──────────────────────────────────────────────────────────
    if (cfg.enemyCount && count > 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Enemies: %d", count);
        ImVec2 ts = ImGui::CalcTextSize(buf);
        dl->AddText({sw*0.5f - ts.x*0.5f, 8.f}, to_col(cfg.enemyCountColor), buf);
    }
}

} // namespace esp
