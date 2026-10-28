// antilose_compat.cpp  —  recode: sync Menu::cfg → cfg::*
#define IMGUI_DEFINE_MATH_OPERATORS
#include "antilose_menu.h"
#include "menu_inflex.h"
#include "imgui.h"
#include "../draw/draw.h"
#include "cfg.hpp"
#include <cmath>

// ── Fonts ─────────────────────────────────────────────────────────────────────
ImFont* Logo     = nullptr;
ImFont* FontLogo = nullptr;
ImFont* Esp      = nullptr;

void AntiloseBindFonts() {
    if (!Logo)     Logo     = fontBold;
    if (!FontLogo) FontLogo = fontBold;
    if (!Esp)      Esp      = espFont;
}

// ── Accent ────────────────────────────────────────────────────────────────────
ImVec4 accentColor = {0.878f,0.149f,0.169f,1.f};

void SyncAccentFromMenu() {
    accentColor = Menu::cfg.col.accent;
}

// ── Radial glow (watermark) ───────────────────────────────────────────────────
void DrawRadialGradientEsp(const ImVec2& center, float radius, ImColor col_in) {
    const float a0 = col_in.Value.w;
    if (a0<=0.003f||radius<0.5f) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const int steps=4;
    for (int i=steps;i>=1;--i) {
        float t=(float)i/(float)steps;
        ImColor c(col_in.Value.x,col_in.Value.y,col_in.Value.z,
                  a0*(1.f-t)*0.55f);
        dl->AddCircleFilled(center,radius*t,c,12);
    }
}

// ── SyncMenuCfg ───────────────────────────────────────────────────────────────
void SyncMenuCfg() {

    // ESP
    cfg::esp::box         = Menu::cfg.boxEsp;
    cfg::esp::name        = Menu::cfg.name;
    cfg::esp::health      = Menu::cfg.healthBar;
    cfg::esp::distance    = Menu::cfg.distance;
    cfg::esp::skeleton    = Menu::cfg.skeleton;
    cfg::esp::snapline    = Menu::cfg.snapline;
    cfg::esp::headdot     = Menu::cfg.headdot;
    cfg::esp::box_type    = Menu::cfg.boxStyle;
    cfg::esp::max_dist    = (float)Menu::cfg.espDist;

    // Aimbot
    cfg::aim::enabled       = Menu::cfg.aimbot;
    cfg::aim::hard_lock     = Menu::cfg.hardLock;
    cfg::aim::silent        = Menu::cfg.silentAim;
    cfg::aim::visible_check = Menu::cfg.visibleCheck;
    cfg::aim::draw_fov      = Menu::cfg.drawFov;   // FIX: was missing — FOV circle never drew
    cfg::aim::fov           = (float)Menu::cfg.fov;
    cfg::aim::smooth        = (float)Menu::cfg.smooth;
    cfg::aim::bone          = Menu::cfg.targetBone;
    cfg::aim::target_mode   = Menu::cfg.targetMode;
    cfg::aim::max_dist      = (float)Menu::cfg.aimMaxDist;


    // Mini Radar
    cfg::radar::enabled   = Menu::cfg.miniRadar;
    cfg::radar::size      = (float)Menu::cfg.radarSize;
    cfg::radar::range     = (float)Menu::cfg.radarRange;
    cfg::radar::show_dist = Menu::cfg.radarShowDist; // FIX: was radarDist (wrong field name)
    cfg::radar::style     = Menu::cfg.radarStyle;

    // World
    cfg::world::potato       = Menu::cfg.potatoGfx;
    cfg::world::fullbright   = Menu::cfg.fullBright;
    cfg::world::time_of_day  = Menu::cfg.timeOfDay;

    // Night Mode
    cfg::nightmode::enabled = Menu::cfg.nightMode;

    // Ambient
    cfg::ambient::enabled   = Menu::cfg.ambientLight;
    cfg::ambient::color     = Menu::cfg.ambientColor; // FIX: color was never synced
    cfg::ambient::intensity = (float)Menu::cfg.ambientIntensity / 100.f;
    cfg::ambient::pulse     = Menu::cfg.ambientPulse;
    cfg::ambient::pulse_spd = (float)Menu::cfg.ambientSpeed / 10.f;

    // Bullet Tracers
    cfg::tracer::enabled   = Menu::cfg.tracerEnabled;
    cfg::tracer::thickness = (float)Menu::cfg.tracerThickness / 10.f;
    cfg::tracer::lifetime  = (float)Menu::cfg.tracerLifetime  / 10.f;
    cfg::tracer::spark     = Menu::cfg.tracerSpark;
    cfg::tracer::fade      = Menu::cfg.tracerFade;
    cfg::tracer::mode      = Menu::cfg.tracerMode; // FIX: tracer mode was never synced

    // Weapon
    cfg::weapon::norecoil  = Menu::cfg.noRecoil;
    cfg::weapon::nospread  = Menu::cfg.noSpread;
    cfg::weapon::rapidfire = Menu::cfg.rapidFire;

    // Misc
    cfg::misc::watermark    = Menu::cfg.watermark;
    cfg::misc::fps_counter  = Menu::cfg.fpsCounter;
    cfg::misc::streamproof  = Menu::cfg.streamproof;
    cfg::misc::fps_limit    = Menu::cfg.fpsLimit;
    cfg::misc::crosshair    = Menu::cfg.crosshair;
    cfg::misc::crosshair_style= Menu::cfg.crosshairStyle;
    cfg::misc::crosshair_size = (float)Menu::cfg.crosshairSize;
    cfg::misc::crosshair_gap  = (float)Menu::cfg.crosshairGap;
    cfg::misc::crosshair_thick= (float)Menu::cfg.crosshairThick / 10.f;
    cfg::misc::hitmarker    = Menu::cfg.hitMarker;
    cfg::misc::hitmarker_size=(float)Menu::cfg.hitMarkerSize;
    cfg::misc::rain         = Menu::cfg.rain;
    cfg::misc::rain_opacity = (float)Menu::cfg.rainOpacity / 100.f;
    cfg::misc::speedhack    = Menu::cfg.speedhack;
    cfg::misc::speed_mult   = Menu::cfg.speedMult;

    // Effects
    cfg::effects::hit_particles = Menu::cfg.hitParticles;
    cfg::effects::death_effect  = Menu::cfg.deathEffect;
    cfg::effects::death_ring    = Menu::cfg.deathRing;

    // ESP Gradient & Glow
    cfg::esp_extra::gradient      = Menu::cfg.espGradient;
    cfg::esp_extra::visible_color = Menu::cfg.espVisibleColor;
    cfg::esp_extra::grad_near     = Menu::cfg.espGradNear;
    cfg::esp_extra::grad_far      = Menu::cfg.espGradFar;
    cfg::esp_extra::visible_col   = Menu::cfg.espVisibleCol;
    cfg::esp_extra::glow          = Menu::cfg.espGlow;
    cfg::esp_extra::glow_size     = (float)Menu::cfg.espGlowSize;
    cfg::esp_extra::glow_alpha    = (float)Menu::cfg.espGlowAlpha / 100.f;

    // Triggerbot
    cfg::settings::triggerbot       = Menu::cfg.triggerbot;
    cfg::settings::tb_visible_check = Menu::cfg.tbVisibleCheck;
    cfg::settings::tb_only_ads      = Menu::cfg.tbOnlyADS;
    cfg::settings::tb_delay         = Menu::cfg.tbDelay;
    cfg::settings::tb_interval      = Menu::cfg.tbInterval;
    cfg::settings::tb_tap_fire      = Menu::cfg.tbTapFire;
    cfg::settings::tb_fire_x_pct    = Menu::cfg.tbFireXPct;
    cfg::settings::tb_fire_y_pct    = Menu::cfg.tbFireYPct;

    // Settings / Items
    cfg::settings::item_esp       = Menu::cfg.itemEsp;
    cfg::settings::weapons        = Menu::cfg.weapons;
    cfg::settings::ammo           = Menu::cfg.ammo;
    cfg::settings::medkits        = Menu::cfg.medkits;
    cfg::settings::item_dist      = Menu::cfg.itemDistance;
    cfg::settings::grenade_warn   = Menu::cfg.grenadeWarning;
    cfg::settings::trajectory     = Menu::cfg.trajectory;
    cfg::settings::prediction     = Menu::cfg.prediction;
}

// ── Unload ────────────────────────────────────────────────────────────────────
static bool g_unload = false;
void RequestUnload()     { g_unload = true; }
bool IsUnloadRequested() { return g_unload; }
