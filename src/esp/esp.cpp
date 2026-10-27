#include "esp.h"
#include "../imgui/imgui.h"
#include <cmath>
#include <cstdio>

float dist3d(const FF::Vec3& a, const FF::Vec3& b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

bool world_to_screen(const Camera& cam, const FF::Vec3& world, FF::Vec2& screen) {
    // Standard MVP projection: clip = proj * view * world
    const float* v = cam.view;
    const float* p = cam.proj;

    // view transform
    float vx = v[0]*world.x + v[4]*world.y + v[8] *world.z + v[12];
    float vy = v[1]*world.x + v[5]*world.y + v[9] *world.z + v[13];
    float vz = v[2]*world.x + v[6]*world.y + v[10]*world.z + v[14];
    float vw = v[3]*world.x + v[7]*world.y + v[11]*world.z + v[15];

    // proj transform
    float cx = p[0]*vx + p[4]*vy + p[8] *vz + p[12]*vw;
    float cy = p[1]*vx + p[5]*vy + p[9] *vz + p[13]*vw;
    float cz = p[2]*vx + p[6]*vy + p[10]*vz + p[14]*vw;
    float cw = p[3]*vx + p[7]*vy + p[11]*vz + p[15]*vw;

    if (cw < 0.001f) return false;
    float ndcx = cx / cw;
    float ndcy = cy / cw;

    screen.x = (ndcx + 1.f) * 0.5f * cam.screen_w;
    screen.y = (1.f - ndcy) * 0.5f * cam.screen_h;
    return true;
}

static ImU32 to_imcol(uint32_t abgr) { return abgr; }

void esp_draw(const std::vector<Entity>& entities,
              const Camera& cam,
              const ESPConfig& cfg) {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    for (const auto& e : entities) {
        if (!e.valid) continue;

        float d = dist3d(e.pos, cam.pos);
        if (d > cfg.max_dist) continue;

        bool is_enemy = (e.team_id != cfg.my_team_id);
        if (!cfg.show_enemies && is_enemy)  continue;
        if (!cfg.show_team   && !is_enemy)  continue;

        ImU32 col = is_enemy ? to_imcol(cfg.color_enemy) : to_imcol(cfg.color_team);

        FF::Vec2 foot, head;
        if (!world_to_screen(cam, e.pos,      foot)) continue;
        if (!world_to_screen(cam, e.head_pos, head)) continue;

        float box_h = fabsf(foot.y - head.y);
        if (box_h < 5.f) continue;
        float box_w = box_h * 0.4f;

        float left  = foot.x - box_w * 0.5f;
        float right = foot.x + box_w * 0.5f;
        float top   = head.y - box_h * 0.1f;
        float bot   = foot.y;

        // 2D box
        if (cfg.box_esp) {
            // Cornered box style (pro kernel look)
            float cs = box_h * 0.2f; // corner segment length
            ImU32 outline = IM_COL32(0,0,0,180);
            float th = 2.f;

            // top-left
            dl->AddLine({left,     top    }, {left+cs,  top    }, outline, th+1);
            dl->AddLine({left,     top    }, {left,     top+cs }, outline, th+1);
            dl->AddLine({left,     top    }, {left+cs,  top    }, col,     th);
            dl->AddLine({left,     top    }, {left,     top+cs }, col,     th);
            // top-right
            dl->AddLine({right-cs, top    }, {right,    top    }, outline, th+1);
            dl->AddLine({right,    top    }, {right,    top+cs }, outline, th+1);
            dl->AddLine({right-cs, top    }, {right,    top    }, col,     th);
            dl->AddLine({right,    top    }, {right,    top+cs }, col,     th);
            // bot-left
            dl->AddLine({left,     bot-cs }, {left,     bot    }, outline, th+1);
            dl->AddLine({left,     bot    }, {left+cs,  bot    }, outline, th+1);
            dl->AddLine({left,     bot-cs }, {left,     bot    }, col,     th);
            dl->AddLine({left,     bot    }, {left+cs,  bot    }, col,     th);
            // bot-right
            dl->AddLine({right,    bot-cs }, {right,    bot    }, outline, th+1);
            dl->AddLine({right-cs, bot    }, {right,    bot    }, outline, th+1);
            dl->AddLine({right,    bot-cs }, {right,    bot    }, col,     th);
            dl->AddLine({right-cs, bot    }, {right,    bot    }, col,     th);
        }

        // HP bar — left side
        if (cfg.health_bar && e.max_hp > 0) {
            float hp_ratio = e.hp / e.max_hp;
            if (hp_ratio > 1.f) hp_ratio = 1.f;
            float bx    = left - 6.f;
            float b_top = top;
            float b_bot = bot;
            float fill  = b_top + (b_bot - b_top) * (1.f - hp_ratio);

            dl->AddRectFilled({bx-1, b_top-1}, {bx+4, b_bot+1}, IM_COL32(0,0,0,180));
            dl->AddRectFilled({bx,   fill    }, {bx+3, b_bot  }, to_imcol(cfg.color_hp));
        }

        // Head dot
        if (cfg.head_dot) {
            dl->AddCircleFilled({head.x, head.y + box_h*0.05f}, 3.f,
                                IM_COL32(0,0,0,160));
            dl->AddCircleFilled({head.x, head.y + box_h*0.05f}, 2.f, col);
        }

        // Name + distance label
        if (cfg.name_tag || cfg.distance_tag) {
            char label[64];
            if (cfg.name_tag && cfg.distance_tag)
                snprintf(label, sizeof(label), "%s [%.0fm]", e.name[0] ? e.name : "?", d);
            else if (cfg.name_tag)
                snprintf(label, sizeof(label), "%s", e.name[0] ? e.name : "?");
            else
                snprintf(label, sizeof(label), "%.0fm", d);

            ImVec2 ts = ImGui::CalcTextSize(label);
            float tx = foot.x - ts.x * 0.5f;
            float ty = bot + 2.f;

            // text shadow
            dl->AddText({tx+1, ty+1}, IM_COL32(0,0,0,200), label);
            dl->AddText({tx,   ty  }, col, label);
        }
    }
}
