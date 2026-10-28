#pragma once
// ════════════════════════════════════════════════════════════════════════════
//  menu_inflex.h  —  INFLEX UI  |  recode by @inflexiblen1
//  Dear ImGui (1.89+), zero external deps except imgui itself.
//
//  Usage:
//    Menu::scale = 1.35f;
//    Menu::LoadFontsMemory(...);   // once, before first frame
//    Menu::Render();               // each frame between NewFrame / Render
//    Read values from Menu::cfg.*
// ════════════════════════════════════════════════════════════════════════════
#include "imgui.h"

namespace Menu {

// ── Accent palette ───────────────────────────────────────────────────────────
struct Colors {
    ImVec4 accent;      // main accent (icons, toggles, sliders, active tab mark)
    ImVec4 tab_text;    // active tab label
    ImVec4 toggle;      // toggle fill
    ImVec4 slider;      // slider fill
};

// ── All settings the menu exposes ────────────────────────────────────────────
struct Settings {

    // ── ESP ──────────────────────────────────────────────────────────────────
    bool boxEsp = true, name = false, healthBar = true,
         distance = false, skeleton = false, hostIndicator = false,
         snapline = false, headdot = false;
    int  boxStyle = 0;          // 0=2D 1=Corner 2=3D
    int  espDist  = 300;        // max ESP distance 50..600
    // HP bar gradient
    bool hpGradient = true;

    // ── ESP Gradient ──────────────────────────────────────────────────────────
    bool  espGradient     = true;   // colour-by-distance gradient on box
    bool  espVisibleColor = true;   // tint visible enemies differently
    ImVec4 espGradNear   = {0.2f, 0.85f, 1.f, 1.f};   // close-range colour
    ImVec4 espGradFar    = {1.f,  0.18f, 0.18f, 1.f};  // far-range colour
    ImVec4 espVisibleCol = {0.3f, 1.f,   0.3f, 1.f};   // visible enemy tint
    // ESP Glow
    bool  espGlow        = false;
    int   espGlowSize    = 8;    // 4..24
    int   espGlowAlpha   = 40;   // 5..80  → /100 → float

    // ── Aimbot ────────────────────────────────────────────────────────────────
    bool aimbot = false, visibleCheck = true, drawFov = false,
         hardLock = true, silentAim = false;
    int  fov         = 90;      // 10..360
    int  smooth      = 6;       // 1..20
    int  targetBone  = 0;       // 0=Head 1=Neck 2=Chest 3=Stomach 4=Pelvis
    int  targetMode  = 0;       // 0=ClosestFOV 1=LowHP 2=Distance
    int  aimMaxDist  = 300;     // 50..600

    // ── Triggerbot ────────────────────────────────────────────────────────────
    bool triggerbot = false, onlyWhenAiming = false;
    int  triggerDelay = 80;     // 0..500 ms
    int  tbDelay         = 80;  // 0..500 ms задержка перед выстрелом
    int  tbInterval      = 120; // 50..600 ms между выстрелами
    bool tbVisibleCheck  = true;
    bool tbOnlyADS       = false;
    bool tbTapFire       = false;
    int tbFireXPct       = 90;
    int tbFireYPct       = 80;

    // ── Weapon ────────────────────────────────────────────────────────────────
    bool noRecoil = false, noSpread = false, rapidFire = false;

    // ── World ─────────────────────────────────────────────────────────────────
    bool fullBright = false, potatoGfx = false, nightMode = false;
    int  timeOfDay = 12;        // 0..24

    // ── Radar ─────────────────────────────────────────────────────────────────
    bool miniRadar = false, radarShowDist = true;
    int  radarSize  = 150;      // 80..300
    int  radarRange = 80;       // 20..200
    int  radarStyle = 0;        // 0=Round 1=Square 2=Arrow

    // ── Ambient ───────────────────────────────────────────────────────────────
    bool ambientLight = false, ambientPulse = true;
    ImVec4 ambientColor = {0.05f, 0.05f, 0.2f, 1.f};
    int  ambientIntensity = 12; // 0..85  → /100 → float
    int  ambientSpeed     = 15; // 5..50  → /10  → float

    // ── Bullet Tracers ────────────────────────────────────────────────────────
    bool tracerEnabled = false, tracerSpark = true, tracerFade = true;
    int  tracerThickness = 15;  // 5..40  → /10 → float
    int  tracerLifetime  = 12;  // 3..30  → /10 → float
    int  tracerMode      = 0;   // 0=2D 1=3D

    // ── Crosshair ─────────────────────────────────────────────────────────────
    bool crosshair = false;
    int  crosshairStyle = 0;    // 0=Cross 1=Dot 2=Circle 3=Cross+Circle
    int  crosshairSize  = 8;    // 2..30
    int  crosshairGap   = 4;    // 0..20
    int  crosshairThick = 15;   // 5..40 → /10 → float

    // ── Hit Marker ────────────────────────────────────────────────────────────
    bool hitMarker = false;
    int  hitMarkerSize = 10;    // 4..30
    bool rain = false;
    int  rainOpacity = 60;      // 10..100

    // ── Effects ───────────────────────────────────────────────────────────────
    bool hitParticles = true;   // blood/spark burst on hit
    bool deathEffect  = true;   // explosion particles on kill
    bool deathRing    = true;   // expanding ring on kill

    // ── Speedhack ─────────────────────────────────────────────────────────────
    bool speedhack = false;
    int  speedMult = 15;        // 10..30 → /10 → float 1.0..3.0

    // ── Misc ──────────────────────────────────────────────────────────────────
    bool watermark = true, fpsCounter = false, streamproof = false;
    int  fpsLimit = 1;          // 0=30 1=60 2=90 3=120 4=144

    // ── Items ─────────────────────────────────────────────────────────────────
    bool itemEsp = false, weapons = true, ammo = false, medkits = false;
    int  itemDistance = 150;    // 10..500
    bool grenadeWarning = true, trajectory = false;
    int  prediction = 1;        // 0=Low 1=Medium 2=High

    // ── Settings ──────────────────────────────────────────────────────────────
    bool boneSize = false, autoWin = false;
    bool hitLogs  = false, hitSound = false;
    int  hitSoundType = 0;      // 0=Click 1=Bell 2=Pop
    bool exitRequested = false; // set true → main loop should unload

    // ── Colors ────────────────────────────────────────────────────────────────
    Colors col = {
        {0.878f, 0.149f, 0.169f, 1.f},  // accent  #e0262b
        {1.f,    1.f,    1.f,    1.f},  // tab_text
        {0.878f, 0.149f, 0.169f, 1.f},  // toggle
        {0.878f, 0.149f, 0.169f, 1.f},  // slider
    };
};

extern Settings cfg;
extern bool     open;
extern float    scale;
extern void   (*OnFolder)();

void LoadFonts(const char* medium, const char* semibold, const char* bold);
void LoadFontsMemory(void* medium, int mSz, void* semibold, int sSz, void* bold, int bSz);
void Render();

} // namespace Menu
