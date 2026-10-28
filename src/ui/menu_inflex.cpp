// ════════════════════════════════════════════════════════════════════════════
//  menu_inflex.cpp  —  INFLEX UI recode  |  @inflexiblen1
// ════════════════════════════════════════════════════════════════════════════
#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu_inflex.h"
#include <vector>
#include <unordered_map>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <algorithm>

namespace Menu {

Settings cfg;
bool     open     = true;
float    scale    = 1.35f;
void   (*OnFolder)() = nullptr;

namespace {

// ── Palette ──────────────────────────────────────────────────────────────────
inline ImU32 Hex(unsigned h, int a = 255) {
    return IM_COL32((h>>16)&255,(h>>8)&255,h&255,a);
}
const ImU32 C_BG      = Hex(0x14151a);
const ImU32 C_SIDE    = Hex(0x111216);
const ImU32 C_BTN     = Hex(0x1c1d23);
const ImU32 C_BTN_H   = Hex(0x242530);
const ImU32 C_TAB     = Hex(0x191a20);
const ImU32 C_PANEL   = Hex(0x1b1c22);
const ImU32 C_TRACK   = Hex(0x272830);
const ImU32 C_TRACK_H = Hex(0x30313a);
const ImU32 C_TXT     = Hex(0x7a7c85);
const ImU32 C_TXT_H   = Hex(0xb0b2ba);
const ImU32 C_DIM     = Hex(0x52545e);
const ImU32 C_LINE    = Hex(0x20212a);
const ImU32 C_BORDER  = Hex(0x26272f);
const ImU32 C_VAL     = Hex(0xcfd0d6);
const ImU32 C_NAV     = Hex(0x8a8c95);
const ImU32 C_NAV_H   = Hex(0xd0d2d9);
const ImU32 C_INPUT   = Hex(0xdcdde2);
const ImU32 C_WHITE   = IM_COL32_WHITE;
const ImU32 C_SEPDARK = Hex(0x1e1f26);
const float PI = 3.14159265f;

ImFont* fMed  = nullptr;
ImFont* fSemi = nullptr;
ImFont* fBold = nullptr;

// ── Helpers ───────────────────────────────────────────────────────────────────
inline float  S(float v)           { return v * scale; }
inline ImVec2 S2(float x, float y) { return {x*scale, y*scale}; }
inline ImU32  U(const ImVec4& v)   { return ImGui::ColorConvertFloat4ToU32(v); }
inline void   PF(ImFont* f)        { ImGui::PushFont(f ? f : ImGui::GetFont()); }

ImU32 Mix(ImU32 a, ImU32 b, float t) {
    ImVec4 A = ImGui::ColorConvertU32ToFloat4(a);
    ImVec4 B = ImGui::ColorConvertU32ToFloat4(b);
    return ImGui::ColorConvertFloat4ToU32({
        A.x+(B.x-A.x)*t, A.y+(B.y-A.y)*t,
        A.z+(B.z-A.z)*t, A.w+(B.w-A.w)*t});
}
ImU32 WithAlpha(ImU32 c, float a) {
    ImVec4 v = ImGui::ColorConvertU32ToFloat4(c);
    v.w *= a;
    return ImGui::ColorConvertFloat4ToU32(v);
}

std::unordered_map<ImGuiID,float> g_anim;
float Anim(ImGuiID id, float target, float spd = 14.f) {
    auto it = g_anim.find(id);
    if (it == g_anim.end()) { g_anim[id] = target; return target; }
    float& v = it->second;
    v += (target-v)*(1.f-expf(-spd*ImGui::GetIO().DeltaTime));
    if (fabsf(target-v) < 0.002f) v = target;
    return v;
}

bool Match(const char* s, const char* f) {
    while (*f==' ') ++f;
    if (!*f) return true;
    for (; *s; ++s) {
        const char* a=s; const char* b=f;
        while (*a && *b && tolower((unsigned char)*a)==tolower((unsigned char)*b)) {++a;++b;}
        if (!*b) return true;
    }
    return false;
}

// ── Icons ─────────────────────────────────────────────────────────────────────
enum Ico {
    I_USERS,   // Players
    I_TARGET,  // Aimbot
    I_GRID,    // Misc
    I_GEAR,    // Settings
    I_FOLDER,
    I_CLOSE,
    I_SEARCH,
    I_LEFT,
    I_RIGHT,
    I_GRENADE, // Items
    I_PLANET,  // World (new)
    I_PULSE,   // Misc extras
};

void DrawIcon(ImDrawList* d, int ico, ImVec2 c, float size, ImU32 col, float th) {
    const float k = size/24.f;
    auto P = [&](float x, float y){ return ImVec2(c.x+(x-12.f)*k, c.y+(y-12.f)*k); };
    switch (ico) {
    case I_USERS:
        d->AddCircle(P(9,7), 4*k, col, 24, th);
        d->PathLineTo(P(2,21)); d->PathLineTo(P(2,19));
        d->PathArcTo(P(6,19),4*k,PI,PI*1.5f,10);
        d->PathArcTo(P(12,19),4*k,PI*1.5f,PI*2.f,10);
        d->PathLineTo(P(16,21)); d->PathStroke(col,0,th);
        d->PathLineTo(P(22,21)); d->PathLineTo(P(22,19));
        d->PathArcTo(P(18,19),4*k,0.f,-1.25f,8);
        d->PathStroke(col,0,th);
        d->PathArcTo(P(15,7),4*k,-1.32f,1.32f,12);
        d->PathStroke(col,0,th);
        break;
    case I_TARGET:
        d->AddCircle(P(12,12),7.5f*k,col,32,th);
        d->AddCircle(P(12,12),3.f*k,col,20,th);
        d->AddLine(P(12,1.5f),P(12,4.5f),col,th);
        d->AddLine(P(12,19.5f),P(12,22.5f),col,th);
        d->AddLine(P(1.5f,12),P(4.5f,12),col,th);
        d->AddLine(P(19.5f,12),P(22.5f,12),col,th);
        break;
    case I_GRENADE:
        d->AddCircle(P(13.5f,15),6.5f*k,col,28,th);
        d->PathLineTo(P(10.5f,9.2f)); d->PathLineTo(P(10.5f,6));
        d->PathLineTo(P(16.5f,6));    d->PathLineTo(P(16.5f,9.2f));
        d->PathStroke(col,0,th);
        d->AddLine(P(5,2.5f),P(5,7.5f),col,th);
        d->AddLine(P(2.5f,5),P(7.5f,5),col,th);
        break;
    case I_GRID:
        for (int i=0;i<4;i++){
            float x=(i%2)?13.5f:3.f, y=(i/2)?13.5f:3.f;
            d->AddRect(P(x,y),P(x+7.5f,y+7.5f),col,2.2f*k,0,th);
        }
        break;
    case I_GEAR: {
        const int teeth=8;
        for (int i=0;i<teeth;i++){
            float a=i*2.f*PI/teeth;
            const float q[4][2]={{7.6f,a-.30f},{10.f,a-.17f},{10.f,a+.17f},{7.6f,a+.30f}};
            for (auto& p:q) d->PathLineTo({c.x+cosf(p[1])*p[0]*k, c.y+sinf(p[1])*p[0]*k});
        }
        d->PathStroke(col,ImDrawFlags_Closed,th);
        d->AddCircle(c,3.f*k,col,20,th);
        break; }
    case I_PLANET:
        d->AddCircle(P(12,12),8*k,col,32,th);
        // rings (tilted ellipse)
        d->PathArcTo(P(12,12),10*k,-0.4f,PI+0.4f,16);
        d->PathStroke(col,0,th*0.7f);
        d->AddLine(P(4,13),P(20,11),col,th*0.6f);
        break;
    case I_PULSE:
        d->AddLine(P(2,12),P(6,12),col,th);
        d->PathLineTo(P(6,12)); d->PathLineTo(P(9,5));
        d->PathLineTo(P(12,19)); d->PathLineTo(P(15,9));
        d->PathLineTo(P(17,14)); d->PathLineTo(P(22,14));
        d->PathStroke(col,0,th);
        break;
    case I_FOLDER: {
        const ImVec2 f[]={P(2.5f,18.5f),P(2.5f,5),P(4,3.5f),P(8.3f,3.5f),P(10.6f,6.3f),
                          P(20,6.3f),P(21.5f,7.8f),P(21.5f,18.5f),P(20,20),P(4,20)};
        for (auto& p:f) d->PathLineTo(p);
        d->PathStroke(col,ImDrawFlags_Closed,th);
        d->AddLine(P(9,13.2f),P(15,13.2f),col,th);
        break; }
    case I_CLOSE:
        d->AddLine(P(6,6),P(18,18),col,th);
        d->AddLine(P(18,6),P(6,18),col,th);
        break;
    case I_SEARCH:
        d->AddCircle(P(11,11),7.f*k,col,28,th);
        d->AddLine(P(16.5f,16.5f),P(21,21),col,th);
        break;
    case I_LEFT:
        d->PathLineTo(P(14.5f,5)); d->PathLineTo(P(8,12)); d->PathLineTo(P(14.5f,19));
        d->PathStroke(col,0,th);
        break;
    case I_RIGHT:
        d->PathLineTo(P(9.5f,5)); d->PathLineTo(P(16,12)); d->PathLineTo(P(9.5f,19));
        d->PathStroke(col,0,th);
        break;
    }
}

void DrawLogo(ImDrawList* d, ImVec2 c, float size) {
    const float k=size/30.f;
    auto P=[&](float x,float y){return ImVec2(c.x+(x-15.f)*k,c.y+(y-15.f)*k);};
    ImVec2 in[4]={P(18.4f,11.6f),P(18.4f,18.4f),P(11.6f,18.4f),P(11.6f,11.6f)};
    d->AddQuadFilled(in[0],in[1],in[2],in[3],C_WHITE);
    d->AddTriangleFilled(in[3],P(15,2),in[0],C_WHITE);
    d->AddTriangleFilled(in[0],P(28,15),in[1],C_WHITE);
    d->AddTriangleFilled(in[1],P(15,28),in[2],C_WHITE);
    d->AddTriangleFilled(in[2],P(2,15),in[3],C_WHITE);
}

// ── Data model ────────────────────────────────────────────────────────────────
enum RowType { R_TOGGLE, R_SELECT, R_SLIDER, R_COLOR, R_BUTTON };
struct Row {
    const char* label;
    RowType     type;
    bool*       b;
    int*        v;
    const char* const* opts;
    int         count, mn, mx;
    ImVec4*     col;
};
Row T  (const char* l,bool* b)                              {return{l,R_TOGGLE,b,nullptr,nullptr,0,0,0,nullptr};}
Row Sel(const char* l,int* v,const char* const* o,int n)    {return{l,R_SELECT,nullptr,v,o,n,0,0,nullptr};}
Row Sl (const char* l,int* v,int mn,int mx)                 {return{l,R_SLIDER,nullptr,v,nullptr,0,mn,mx,nullptr};}
Row Col(const char* l,ImVec4* c)                             {return{l,R_COLOR, nullptr,nullptr,nullptr,0,0,0,c};}
Row Btn(const char* l)                                       {return{l,R_BUTTON,nullptr,nullptr,nullptr,0,0,0,nullptr};}
struct Tab     { const char* name; std::vector<Row> rows; };
struct Section { int icon; const char* label; std::vector<Tab> tabs; };

// Option arrays
const char* kBoxStyle[]    = {"2D","Corner","3D"};
const char* kMaterial[]    = {"Flat","Glow","Wireframe"};
const char* kBone[]        = {"Head","Neck","Chest","Stomach","Pelvis"};
const char* kTargetMode[]  = {"Closest FOV","Low HP","Distance"};
const char* kPrediction[]  = {"Low","Medium","High"};
const char* kFps[]         = {"30","60","90","120","144"};
const char* kSound[]       = {"Click","Bell","Pop"};
const char* kCrosshair[]   = {"Cross","Dot","Circle","Cross+Circle"};
const char* kTracerMode[]  = {"2D","3D"};
const char* kRadarStyle[]  = {"Round","Square","Arrow"};
#define OPT(a) a,IM_ARRAYSIZE(a)

std::vector<Section>& Sections() {
    static std::vector<Section> s = {

    // ── Players ──────────────────────────────────────────────────────────────
    { I_USERS, "Players", {
        { "Visuals", {
            T("Box ESP",       &cfg.boxEsp),
            Sel("Box Style",   &cfg.boxStyle,   OPT(kBoxStyle)),
            T("Name",          &cfg.name),
            T("Health Bar",    &cfg.healthBar),
            T("HP Gradient",   &cfg.hpGradient),
            T("Distance",      &cfg.distance),
            Sl("Max Distance", &cfg.espDist, 50, 600),
            T("Skeleton",      &cfg.skeleton),
            T("Snapline",      &cfg.snapline),
            T("Head Dot",      &cfg.headdot),
            T("Host Indicator",&cfg.hostIndicator),
        }},
        { "Gradient", {
            T("Box Gradient",     &cfg.espGradient),
            Col("Near Color",     &cfg.espGradNear),
            Col("Far Color",      &cfg.espGradFar),
            T("Visible Tint",     &cfg.espVisibleColor),
            Col("Visible Color",  &cfg.espVisibleCol),
            T("Box Glow",         &cfg.espGlow),
            Sl("Glow Size",       &cfg.espGlowSize,   4, 24),
            Sl("Glow Alpha",      &cfg.espGlowAlpha,  5, 80),
        }},
        { "World", {
            T("Full Bright",    &cfg.fullBright),
            Sl("Time of Day",   &cfg.timeOfDay,  0, 24),
            T("Potato GFX",     &cfg.potatoGfx),
            T("Night Mode",     &cfg.nightMode),
            T("Mini Radar",     &cfg.miniRadar),
            Sl("Radar Size",    &cfg.radarSize,  80, 300),
            Sl("Radar Range",   &cfg.radarRange, 20, 200),
            T("Show Distance",  &cfg.radarShowDist),
            Sel("Radar Style",  &cfg.radarStyle, OPT(kRadarStyle)),
            T("Ambient Light",  &cfg.ambientLight),
            Col("Ambient Color", &cfg.ambientColor),
            Sl("Intensity",     &cfg.ambientIntensity, 0, 85),
            T("Pulse",          &cfg.ambientPulse),
            Sl("Pulse Speed",   &cfg.ambientSpeed,  5, 50),
            T("Bullet Tracers", &cfg.tracerEnabled),
            Sl("Thickness",     &cfg.tracerThickness, 5, 40),
            Sl("Lifetime",      &cfg.tracerLifetime,  3, 30),
            T("Impact Spark",   &cfg.tracerSpark),
            T("Fade Out",       &cfg.tracerFade),
            Sel("Tracer Mode",  &cfg.tracerMode,  OPT(kTracerMode)),
        }},
    }},

    // ── Aimbot ───────────────────────────────────────────────────────────────
    { I_TARGET, "Aimbot", {
        { "Aimbot", {
            T("Enable Aimbot",  &cfg.aimbot),
            T("Hard Lock",      &cfg.hardLock),
            T("Silent Aim",     &cfg.silentAim),
            T("Visible Check",  &cfg.visibleCheck),
            T("Draw FOV",       &cfg.drawFov),
            Sl("FOV",           &cfg.fov,        10, 360),
            Sl("Smooth",        &cfg.smooth,      1,  20),
            Sl("Max Distance",  &cfg.aimMaxDist, 50, 600),
            Sel("Target Bone",  &cfg.targetBone, OPT(kBone)),
            Sel("Target Mode",  &cfg.targetMode, OPT(kTargetMode)),
        }},
        { "Triggerbot", {
            T("Enable",            &cfg.triggerbot),
            Sl("Delay (ms)",       &cfg.triggerDelay,    0, 500),
            Sl("Interval (ms)",    &cfg.tbInterval,      50, 600),
            T("Visible Check",     &cfg.tbVisibleCheck),
            T("ADS Only",          &cfg.tbOnlyADS),
            T("Tap Fire (root)",   &cfg.tbTapFire),
            Sl("Fire X (%)",       &cfg.tbFireXPct, 0, 100),
            Sl("Fire Y (%)",       &cfg.tbFireYPct, 0, 100),
        }},
        { "Weapon", {
            T("No Recoil",  &cfg.noRecoil),
            T("No Spread",  &cfg.noSpread),
            T("Rapid Fire", &cfg.rapidFire),
        }},
    }},

    // ── Items ─────────────────────────────────────────────────────────────────
    { I_GRENADE, "Items", {
        { "Items", {
            T("Item ESP",       &cfg.itemEsp),
            T("Weapons",        &cfg.weapons),
            T("Ammo",           &cfg.ammo),
            T("Medkits",        &cfg.medkits),
            Sl("Max Distance",  &cfg.itemDistance, 10, 500),
        }},
        { "Throwables", {
            T("Grenade Warning",&cfg.grenadeWarning),
            T("Trajectory",     &cfg.trajectory),
            Sel("Prediction",   &cfg.prediction,    OPT(kPrediction)),
        }},
    }},

    // ── Misc ──────────────────────────────────────────────────────────────────
    { I_GRID, "Misc", {
        { "General", {
            T("Watermark",      &cfg.watermark),
            T("FPS Counter",    &cfg.fpsCounter),
            Sel("FPS Limit",    &cfg.fpsLimit,   OPT(kFps)),
            T("Streamproof",    &cfg.streamproof),
        }},
        { "Crosshair", {
            T("Enable",         &cfg.crosshair),
            Sel("Style",        &cfg.crosshairStyle, OPT(kCrosshair)),
            Sl("Size",          &cfg.crosshairSize,  2, 30),
            Sl("Gap",           &cfg.crosshairGap,   0, 20),
            Sl("Thickness",     &cfg.crosshairThick, 5, 40),
        }},
        { "Hit Marker", {
            T("Enable",         &cfg.hitMarker),
            Sl("Size",          &cfg.hitMarkerSize,  4, 30),
        }},
        { "Speedhack", {
            T("Enable",         &cfg.speedhack),
            Sl("Speed x",       &cfg.speedMult, 10, 30),
        }},
        { "Effects", {
            T("Rain",           &cfg.rain),
            Sl("Rain Opacity",  &cfg.rainOpacity, 10, 100),
            T("Hit Particles",  &cfg.hitParticles),
            T("Death Effect",   &cfg.deathEffect),
            T("Death Ring",     &cfg.deathRing),
        }},
    }},

    // ── Settings ──────────────────────────────────────────────────────────────
    { I_GEAR, "Settings", {
        { "Exploits", {
            T("Bone Size", &cfg.boneSize),
            T("Auto Win (HOST only!)", &cfg.autoWin),
        }},
        { "Hit Data", {
            T("Hit Logs",   &cfg.hitLogs),
            T("Hit Sound",  &cfg.hitSound),
            Sel("Sound",    &cfg.hitSoundType, OPT(kSound)),
        }},
        { "System", {
            Btn("Exit Cheat"),
        }},
    }},

    };
    return s;
}

// ── Scroll state ──────────────────────────────────────────────────────────────
int   g_cur  = 0;
int   g_tab[5] = {};
char  g_query[64] = "";
bool  g_gearWasOpen = false;
float g_scroll[5][8] = {};

// ── Widgets ───────────────────────────────────────────────────────────────────
bool IconButton(const char* id, ImVec2 pos, ImVec2 size, int ico,
                ImU32 idle, float th, bool forceOn = false) {
    ImGui::SetCursorScreenPos(pos);
    bool pressed = ImGui::InvisibleButton(id, size);
    bool hov = ImGui::IsItemHovered(), act = ImGui::IsItemActive();
    float h = Anim(ImGui::GetID(id), (hov||forceOn)?1.f:0.f);
    ImDrawList* d = ImGui::GetWindowDrawList();
    float sc = act?0.94f:1.f;
    ImVec2 c=pos+size*0.5f, half=size*0.5f*sc;
    d->AddRectFilled(c-half, c+half, Mix(C_BTN,C_BTN_H,h), S(9)*sc);
    DrawIcon(d, ico, c, S(19)*sc, Mix(idle,forceOn?C_NAV_H:C_NAV,h), th);
    return pressed;
}

void Toggle(ImVec2 pos, bool* v) {
    ImDrawList* d = ImGui::GetWindowDrawList();
    ImVec2 sz = S2(44,23);
    ImGui::SetCursorScreenPos(pos);
    if (ImGui::InvisibleButton("##tg",sz)) *v=!*v;
    float t = Anim(ImGui::GetID("##tg"),*v?1.f:0.f,12.f);
    // track
    d->AddRectFilled(pos, pos+sz, Mix(C_TRACK,U(cfg.col.toggle),t), sz.y*0.5f);
    // thumb shadow
    float cx = pos.x+S(3)+Anim(ImGui::GetID("##tgs"),*v?1.f:0.f,12.f)*S(21);
    float cy = pos.y+sz.y*0.5f;
    float r  = S(8.5f);
    d->AddCircleFilled({cx+r,cy}, r+1.f, IM_COL32(0,0,0,60), 24);
    d->AddCircleFilled({cx+r,cy}, r,     C_WHITE, 24);
}

bool ArrowButton(const char* id, ImVec2 pos, bool right) {
    ImDrawList* d = ImGui::GetWindowDrawList();
    ImVec2 sz = S2(23,23);
    ImGui::SetCursorScreenPos(pos);
    bool pressed = ImGui::InvisibleButton(id,sz);
    bool hov=ImGui::IsItemHovered(), act=ImGui::IsItemActive();
    float h = Anim(ImGui::GetID(id),hov?1.f:0.f);
    float sc = act?0.9f:1.f;
    ImVec2 c=pos+sz*0.5f, half=sz*0.5f*sc;
    d->AddRectFilled(c-half, c+half, Mix(C_TRACK,C_TRACK_H,h), S(7)*sc);
    DrawIcon(d, right?I_RIGHT:I_LEFT, c, S(12)*sc, Mix(C_NAV,C_WHITE,h), S(1.8f));
    return pressed;
}

void Selector(float right, float cy, int* idx, const char* const* opts, int n) {
    ImDrawList* d = ImGui::GetWindowDrawList();
    float top = cy-S(11.5f);
    if (ArrowButton("##l", {right-S(122),top}, false)) *idx=(*idx-1+n)%n;
    if (ArrowButton("##r", {right-S(23), top}, true))  *idx=(*idx+1)%n;
    const char* txt = opts[*idx];
    ImVec2 ts = ImGui::CalcTextSize(txt);
    float cx = right-S(23)-S(6)-S(32);
    d->AddText({cx-ts.x*0.5f, cy-ts.y*0.5f}, C_VAL, txt);
}

void Slider(float right, float cy, int* v, int mn, int mx) {
    ImDrawList* d = ImGui::GetWindowDrawList();
    const float w=S(170), h=S(5);
    const float x0=right-S(210);
    ImGui::SetCursorScreenPos({x0-S(7), cy-S(9)});
    ImGui::InvisibleButton("##sl", {w+S(14),S(18)});
    bool act = ImGui::IsItemActive();
    if (act) {
        float p=(ImGui::GetIO().MousePos.x-x0)/w;
        p=p<0.f?0.f:(p>1.f?1.f:p);
        *v=mn+(int)lroundf(p*(float)(mx-mn));
    }
    float p  = (float)(*v-mn)/(float)(mx-mn);
    float pa = Anim(ImGui::GetID("##slA"), p, 24.f);

    // track
    d->AddRectFilled({x0,cy-h*0.5f},{x0+w,cy+h*0.5f}, C_TRACK, h*0.5f);
    // fill
    if (w*pa>1.f) {
        // gradient fill
        ImU32 col0 = U(cfg.col.slider);
        ImU32 col1 = WithAlpha(col0, 0.5f);
        d->AddRectFilledMultiColor({x0,cy-h*0.5f},{x0+w*pa,cy+h*0.5f},
            col1, col0, col0, col1);
    }
    // thumb glow
    float tx = x0+w*pa;
    if (act) {
        d->AddCircleFilled({tx,cy}, S(9.5f), WithAlpha(U(cfg.col.slider),0.22f), 16);
    }
    d->AddCircleFilled({tx,cy}, S(6.5f)*(act?1.15f:1.f), C_WHITE, 20);

    char buf[16]; snprintf(buf,sizeof buf,"%d",*v);
    ImVec2 ts = ImGui::CalcTextSize(buf);
    d->AddText({right-ts.x, cy-ts.y*0.5f}, C_VAL, buf);
}

// ── DrawRows ──────────────────────────────────────────────────────────────────
void DrawRows(std::vector<Row>& rows, float& scroll_off) {
    ImDrawList* d = ImGui::GetWindowDrawList();
    const float rowH   = S(44);
    const float panelH = S(340);
    const float SB_W   = S(3);
    const float SB_PAD = S(4);

    std::vector<Row*> vis;
    for (auto& r : rows) if (Match(r.label,g_query)) vis.push_back(&r);

    ImVec2 p  = ImGui::GetCursorScreenPos();
    float  w  = ImGui::GetContentRegionAvail().x;

    if (vis.empty()) {
        d->AddRectFilled(p, p+ImVec2(w,rowH), C_PANEL, S(9));
        char msg[96]; snprintf(msg,sizeof msg,"No results for \"%s\"",g_query);
        d->AddText({p.x+S(15),p.y+(rowH-ImGui::GetFontSize())*0.5f}, C_DIM, msg);
        ImGui::Dummy({w,rowH});
        scroll_off=0.f;
        return;
    }

    float total_h    = rowH*(float)vis.size();
    float max_scroll = total_h>panelH ? total_h-panelH : 0.f;
    float visible_h  = total_h<panelH ? total_h : panelH;
    bool  has_sb     = max_scroll>0.f;
    float row_w      = w-(has_sb ? SB_W+SB_PAD*2.f+S(2) : 0.f);

    // mousewheel
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel!=0.f) scroll_off -= wheel*rowH*0.75f;
    }
    scroll_off = scroll_off<0.f?0.f:(scroll_off>max_scroll?max_scroll:scroll_off);

    // scrollbar drag (before drawlist ops)
    float sb_x     = p.x+w-SB_W-SB_PAD;
    float track_y0 = p.y+S(8);
    float track_y1 = p.y+visible_h-S(8);
    float track_h  = track_y1-track_y0;
    bool  sb_hov=false, sb_act=false;
    if (has_sb && track_h>1.f) {
        ImGui::SetCursorScreenPos({sb_x-S(4), track_y0});
        ImGui::InvisibleButton("##sb", {SB_W+S(8),track_h});
        sb_hov=ImGui::IsItemHovered();
        sb_act=ImGui::IsItemActive();
        if (sb_act) {
            float delta=ImGui::GetIO().MouseDelta.y;
            if (delta!=0.f) scroll_off+=delta*(total_h/panelH);
            scroll_off=scroll_off<0.f?0.f:(scroll_off>max_scroll?max_scroll:scroll_off);
        }
    }

    // background
    d->AddRectFilled(p, p+ImVec2(w,visible_h), C_PANEL, S(10));
    // subtle top/bottom inner shadow
    d->AddRectFilled(p, p+ImVec2(w,S(4)),
        IM_COL32(0,0,0,40), 0.f);
    d->AddRectFilled(p+ImVec2(0,visible_h-S(4)), p+ImVec2(w,visible_h),
        IM_COL32(0,0,0,40), 0.f);

    // ── Pass 1: DrawList text + dividers ─────────────────────────────────────
    d->PushClipRect(p, p+ImVec2(row_w,visible_h), true);
    for (size_t i=0;i<vis.size();i++) {
        float ry = p.y+rowH*(float)i-scroll_off;
        if (ry+rowH<=p.y||ry>=p.y+panelH) continue;

        // row highlight on hover — use ImGui rect check
        ImVec2 r_min{p.x, ry}, r_max{p.x+row_w, ry+rowH};
        bool row_hov = ImGui::IsMouseHoveringRect(r_min, r_max, false);
        if (row_hov) d->AddRectFilled(r_min, r_max, IM_COL32(255,255,255,7));

        if (i>0) {
            float sep_y = ry;
            d->AddLine({p.x+S(12),sep_y},{p.x+row_w-S(12),sep_y},
                IM_COL32(255,255,255,8), 1.f);
        }
        float cy2 = ry+rowH*0.5f;
        d->AddText({p.x+S(15), cy2-ImGui::GetFontSize()*0.5f},
            row_hov ? C_TXT_H : C_TXT, vis[i]->label);
    }
    d->PopClipRect();

    // ── Pass 2: ImGui widgets ─────────────────────────────────────────────────
    for (size_t i=0;i<vis.size();i++) {
        float ry = p.y+rowH*(float)i-scroll_off;
        float cy2 = ry+rowH*0.5f;
        if (cy2<=p.y||cy2>=p.y+panelH) continue;
        Row& r   = *vis[i];
        float right = p.x+row_w-S(17);
        ImGui::PushID((int)i);
        switch (r.type) {
        case R_TOGGLE: Toggle({right-S(44),cy2-S(11.5f)}, r.b); break;
        case R_BUTTON: {
            // Danger-style exit button
            ImVec2 bsz = S2(90,24);
            ImVec2 bp  = {right-bsz.x, cy2-bsz.y*0.5f};
            ImGui::SetCursorScreenPos(bp);
            if (ImGui::InvisibleButton("##exitbtn", bsz)) {
                // find exitRequested in cfg and set it
                cfg.exitRequested = true;
            }
            bool bh2 = ImGui::IsItemHovered(), ba = ImGui::IsItemActive();
            float bani = Anim(ImGui::GetID("##exitbtn"), bh2?1.f:0.f, 12.f);
            ImU32 base_c  = IM_COL32(120,18,18,255);
            ImU32 hover_c = IM_COL32(200,30,30,255);
            ImU32 btn_col = Mix(base_c, hover_c, bani);
            d->AddRectFilled(bp, bp+bsz, btn_col, S(7));
            d->AddRect(bp, bp+bsz, IM_COL32(255,80,80,(int)(80+100*bani)), S(7), 0, 1.f);
            const char* etxt = "EXIT CHEAT";
            PF(fSemi);
            ImVec2 ets = ImGui::CalcTextSize(etxt);
            d->AddText({bp.x+(bsz.x-ets.x)*0.5f, cy2-ets.y*0.5f},
                IM_COL32(255,200,200,255), etxt);
            ImGui::PopFont();
            break;
        }
        case R_SELECT: Selector(right,cy2,r.v,r.opts,r.count); break;
        case R_SLIDER: Slider(right,cy2,r.v,r.mn,r.mx); break;
        case R_COLOR: {
            // Inline compact color button + picker popup
            ImDrawList* dc = ImGui::GetWindowDrawList();
            ImVec2 cbp = {right-S(44), cy2-S(11.5f)};
            ImVec2 cbsz = S2(44,23);
            ImGui::SetCursorScreenPos(cbp);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, S(6));
            if (ImGui::ColorButton("##cb", *r.col,
                ImGuiColorEditFlags_NoTooltip|ImGuiColorEditFlags_NoAlpha, cbsz))
                ImGui::OpenPopup("##cpop");
            ImGui::PopStyleVar();
            dc->AddRect(cbp, cbp+cbsz, Hex(0x34353c), S(6));
            if (ImGui::BeginPopup("##cpop")) {
                ImGui::SetNextItemWidth(S(180));
                ImGui::ColorPicker3("##cpk", (float*)r.col,
                    ImGuiColorEditFlags_NoSidePreview|
                    ImGuiColorEditFlags_NoSmallPreview|
                    ImGuiColorEditFlags_PickerHueBar);
                ImGui::EndPopup();
            }
            break;
        }
        }
        ImGui::PopID();
    }

    // ── Scrollbar draw ────────────────────────────────────────────────────────
    if (has_sb && track_h>1.f) {
        // track
        d->AddRectFilled({sb_x,track_y0},{sb_x+SB_W,track_y1},
            IM_COL32(255,255,255,15), SB_W*0.5f);
        // thumb
        float thumb_h = (panelH/total_h)*track_h;
        if (thumb_h<S(16)) thumb_h=S(16);
        float thumb_t  = scroll_off/max_scroll;
        float thumb_y0 = track_y0+thumb_t*(track_h-thumb_h);
        ImU32 tc = sb_act  ? IM_COL32(255,255,255,220)
                 : sb_hov  ? IM_COL32(255,255,255,160)
                           : IM_COL32(255,255,255,75);
        d->AddRectFilled({sb_x,thumb_y0},{sb_x+SB_W,thumb_y0+thumb_h},tc,SB_W*0.5f);
    }

    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy({w,visible_h});
}

// ── Color row ─────────────────────────────────────────────────────────────────
void ColorRow(const char* label, ImVec4* c) {
    ImDrawList* d = ImGui::GetWindowDrawList();
    ImVec2 p  = ImGui::GetCursorScreenPos();
    float  w  = ImGui::GetContentRegionAvail().x;
    float  h  = S(32);
    d->AddText({p.x,p.y+(h-ImGui::GetFontSize())*0.5f}, C_TXT, label);
    ImGui::PushID(label);
    ImVec2 sw(p.x+w-S(30), p.y+(h-S(18))*0.5f);
    ImGui::SetCursorScreenPos(sw);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, S(6));
    if (ImGui::ColorButton("##sw",*c,
        ImGuiColorEditFlags_NoTooltip|ImGuiColorEditFlags_NoAlpha, S2(30,18)))
        ImGui::OpenPopup("##picker");
    ImGui::PopStyleVar();
    d->AddRect(sw,sw+S2(30,18),Hex(0x34353c),S(6));
    if (ImGui::BeginPopup("##picker")) {
        ImGui::SetNextItemWidth(S(180));
        ImGui::ColorPicker3("##pk",(float*)c,
            ImGuiColorEditFlags_NoSidePreview|
            ImGuiColorEditFlags_NoSmallPreview|
            ImGuiColorEditFlags_PickerHueBar);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy({w,h});
}

// ── Colors popup ──────────────────────────────────────────────────────────────
void ColorsPopup(ImVec2 origin) {
    ImGui::SetNextWindowPos(origin+S2(96,50));
    ImGui::SetNextWindowSize({S(240),0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   S2(12,12));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding,   S(11));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,     {0,0});
    ImGui::PushStyleColor(ImGuiCol_PopupBg, C_PANEL);
    ImGui::PushStyleColor(ImGuiCol_Border,  C_BORDER);

    if (ImGui::BeginPopup("##colors")) {
        ImDrawList* d = ImGui::GetWindowDrawList();
        PF(fBold);
        ImGui::PushStyleColor(ImGuiCol_Text,C_WHITE);
        ImGui::TextUnformatted("Theme");
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::Dummy(S2(0,4));
        PF(fMed);
        ColorRow("Accent",    &cfg.col.accent);
        ColorRow("Tab text",  &cfg.col.tab_text);
        ColorRow("Toggles",   &cfg.col.toggle);
        ColorRow("Sliders",   &cfg.col.slider);
        ImGui::PopFont();

        // presets
        ImVec2 pp = ImGui::GetCursorScreenPos();
        float  pw = ImGui::GetContentRegionAvail().x;
        d->AddLine({pp.x,pp.y+S(6)},{pp.x+pw,pp.y+S(6)},C_BORDER,1.f);
        float ey = pp.y+S(16);
        static const unsigned presets[8] = {
            0xe0262b,0xff7a1a,0xf2c230,0x3ccf6e,
            0x25c4d9,0x3b82f6,0x8b5cf6,0xec4899};
        for (int i=0;i<8;i++) {
            ImVec2 pos(pp.x+S(23)*i, ey);
            ImGui::PushID(i);
            ImGui::SetCursorScreenPos(pos);
            if (ImGui::InvisibleButton("##pre",S2(17,17))) {
                ImVec4 nc=ImGui::ColorConvertU32ToFloat4(Hex(presets[i]));
                cfg.col.accent=cfg.col.toggle=cfg.col.slider=nc;
            }
            float k=ImGui::IsItemActive()?0.9f:(ImGui::IsItemHovered()?1.15f:1.f);
            ImVec2 ec=pos+S2(8.5f,8.5f);
            d->AddCircleFilled(ec,S(8.5f)*k,Hex(presets[i]),24);
            d->AddCircle(ec,S(8.5f)*k-1.f,IM_COL32(0,0,0,64),24,2.f);
            ImGui::PopID();
        }
        // reset
        PF(fSemi);
        const char* rs="Reset";
        ImVec2 rts=ImGui::CalcTextSize(rs);
        ImVec2 rpos(pp.x+pw-rts.x, ey+(S(17)-rts.y)*0.5f);
        ImGui::SetCursorScreenPos(rpos);
        if (ImGui::InvisibleButton("##reset",rts))
            cfg.col=Settings().col;
        d->AddText(rpos,ImGui::IsItemHovered()?C_NAV_H:C_DIM,rs);
        ImGui::PopFont();
        ImGui::SetCursorScreenPos({pp.x,ey});
        ImGui::Dummy({pw,S(17)});
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
}

} // anonymous namespace

// ── Public ────────────────────────────────────────────────────────────────────
void LoadFonts(const char* med, const char* semi, const char* bold) {
    ImGuiIO& io=ImGui::GetIO();
    ImFontConfig fc; fc.OversampleH=3; fc.OversampleV=2;
    auto load=[&](const char* path,float sz)->ImFont*{
        return path?io.Fonts->AddFontFromFileTTF(path,sz*scale,&fc):nullptr;};
    fMed  = load(med,  12.6f);
    fSemi = load(semi, 11.5f);
    fBold = load(bold, 11.5f);
}
void LoadFontsMemory(void* med,int mSz,void* semi,int sSz,void* bold,int bSz){
    ImGuiIO& io=ImGui::GetIO();
    ImFontConfig fc; fc.OversampleH=3; fc.OversampleV=2;
    fc.FontDataOwnedByAtlas=false;
    auto load=[&](void* data,int sz,float size)->ImFont*{
        return data?io.Fonts->AddFontFromMemoryTTF(data,sz,size*scale,&fc):nullptr;};
    fMed  = load(med,  mSz, 12.6f);
    fSemi = load(semi, sSz, 11.5f);
    fBold = load(bold, bSz, 11.5f);
}

void Render() {
    if (ImGui::IsKeyPressed(ImGuiKey_Insert,false)) open=!open;
    if (!open) return;

    const ImVec2 size = S2(720,468);  // немного шире + выше для новых вкладок
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
        ImGuiCond_FirstUseEver,{0.5f,0.5f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   {0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::Begin("##inflex_v2",nullptr,
        ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
        ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse|
        ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoBackground|
        ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar(2);

    ImDrawList* d = ImGui::GetWindowDrawList();
    const ImVec2 o = ImGui::GetWindowPos();

    // ── Drop shadow ───────────────────────────────────────────────────────────
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    for (int i=1;i<=7;i++) {
        float e=S(2.5f*i);
        bg->AddRectFilled(o-ImVec2(e,e)+ImVec2(0,S(8)),
                          o+size+ImVec2(e,e)+ImVec2(0,S(8)),
                          IM_COL32(0,0,0,9), S(20)+e);
    }

    // ── Window body ───────────────────────────────────────────────────────────
    d->AddRectFilled(o, o+size, C_BG, S(18));
    d->AddRect(o, o+size, C_LINE, S(18), 0, 1.f);

    // ── Sidebar ───────────────────────────────────────────────────────────────
    const ImVec2 sp = o+S2(9,9);
    d->AddRectFilled(sp, sp+S2(80,450), C_SIDE, S(12));

    // Logo
    DrawLogo(d, sp+S2(40,32), S(30));

    // Thin accent line under logo
    float lx = sp.x+S(14);
    d->AddLine({lx,sp.y+S(56)},{lx+S(52),sp.y+S(56)},
        WithAlpha(U(cfg.col.accent),0.3f), 1.f);

    // Nav section labels (tiny, above icon groups)
    auto& secs = Sections();
    const float navY[5] = {78,150,221,292,363};

    for (int i=0;i<(int)secs.size();i++) {
        ImVec2 bp = sp+S2(0,navY[i]);
        ImGui::PushID(i);
        ImGui::SetCursorScreenPos(bp);
        if (ImGui::InvisibleButton("##nav",S2(80,44)) && g_cur!=i) {
            g_cur=i;
            g_query[0]=0;
            g_scroll[g_cur][g_tab[g_cur]]=0.f;
        }
        bool hov=ImGui::IsItemHovered(), act=ImGui::IsItemActive();
        float h  = Anim(ImGui::GetID("##nh"),hov?1.f:0.f, 8.f);
        float a  = Anim(ImGui::GetID("##na"),g_cur==i?1.f:0.f);
        ImU32 ic = Mix(Mix(C_NAV,C_NAV_H,h),U(cfg.col.accent),a);

        // hover bg — only when clearly active or hovered
        if (h>0.05f||a>0.05f) {
            float ba=a>h?a:h;
            d->AddRectFilled(bp+S2(6,4),bp+S2(74,40),
                WithAlpha(U(cfg.col.accent),0.09f*ba),S(8));
        }

        DrawIcon(d, secs[i].icon, bp+S2(40,18), S(20)*(act?0.9f:1.f), ic, S(1.7f));

        // Section label below icon
        PF(fSemi);
        ImVec2 lts = ImGui::CalcTextSize(secs[i].label);
        d->AddText({bp.x+S(40)-lts.x*0.5f, bp.y+S(28)},
            WithAlpha(ic, 0.65f+0.35f*a), secs[i].label);
        ImGui::PopFont();

        // Active indicator bar (left side)
        if (a>0.01f) {
            float bar_y0 = bp.y+S(10);
            float bar_y1 = bp.y+S(34);
            d->AddRectFilled({sp.x+S(2),bar_y0},{sp.x+S(4),bar_y1},
                WithAlpha(U(cfg.col.accent),a), S(2));
        }
        ImGui::PopID();
    }

    // ── Top bar ───────────────────────────────────────────────────────────────
    // Gear / colors toggle
    bool popOpen = ImGui::IsPopupOpen("##colors");
    bool gearPressed = IconButton("##gear",o+S2(98,10),S2(36,36),
        I_GEAR,C_DIM,S(1.8f),popOpen);
    if (ImGui::IsItemActivated()) g_gearWasOpen=popOpen;
    if (gearPressed && !g_gearWasOpen) ImGui::OpenPopup("##colors");

    // Search bar
    {
        ImVec2 sb=o+S2(142,10), ss=S2(479,36);
        d->AddRectFilled(sb,sb+ss,C_BTN,S(9));
        // search icon
        DrawIcon(d, I_SEARCH, sb+ImVec2(S(19),ss.y*0.5f), S(18),C_TXT,S(1.9f));
        PF(fMed);
        float fh=ImGui::GetFontSize();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{0,(ss.y-fh)*0.5f});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0.f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg,        IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_Text,           C_INPUT);
        ImGui::PushStyleColor(ImGuiCol_TextDisabled,   C_TXT);
        ImGui::SetCursorScreenPos(sb+ImVec2(S(36),0));
        ImGui::SetNextItemWidth(ss.x-S(46));
        ImGui::InputTextWithHint("##search","Search settings...",g_query,sizeof g_query);
        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(2);
        ImGui::PopFont();
    }

    if (IconButton("##folder",o+S2(630,10),S2(36,36),I_FOLDER,C_TXT,S(1.8f))&&OnFolder)
        OnFolder();
    if (IconButton("##close", o+S2(675,10),S2(36,36),I_CLOSE,Hex(0xa8aab0),S(2)))
        open=false;

    // ── Separator under top bar ───────────────────────────────────────────────
    d->AddLine(o+S2(98,54), o+S2(622,54), C_LINE, 1.f);

    // ── Tabs ──────────────────────────────────────────────────────────────────
    Section& sec = secs[g_cur];
    const int tab_count = (int)sec.tabs.size();
    // Distribute tabs evenly
    float tab_area_w = S(622-98);
    float tab_w = tab_area_w/(float)tab_count;

    for (int i=0;i<tab_count;i++) {
        ImVec2 tp = o+ImVec2(S(98)+tab_w*(float)i, S(56));
        ImVec2 ts = {tab_w, S(36)};
        ImGui::PushID(100+i);
        ImGui::SetCursorScreenPos(tp);
        if (ImGui::InvisibleButton("##tab",ts)) {
            g_tab[g_cur]=i;
            g_scroll[g_cur][i]=0.f;
        }
        bool on=g_tab[g_cur]==i;
        bool hov=ImGui::IsItemHovered(), act=ImGui::IsItemActive();
        float a=Anim(ImGui::GetID("##ta"),on?1.f:0.f);
        float h=Anim(ImGui::GetID("##th"),(hov&&!on)?1.f:0.f,10.f);
        float sc=act?0.97f:1.f;
        ImVec2 tc=tp+ts*0.5f, th2=ts*0.5f*sc;
        d->AddRectFilled(tc-th2,tc+th2, Mix(Mix(C_TAB,C_BTN,h),C_BTN,a),S(8));

        // Active underline
        if (a>0.01f) {
            float ux0=tp.x+tab_w*0.25f, ux1=tp.x+tab_w*0.75f;
            float uy=tp.y+ts.y-S(2);
            d->AddRectFilled({ux0,uy},{ux1,uy+S(2)},
                WithAlpha(U(cfg.col.accent),a),S(1));
        }

        PF(on?fBold:fSemi);
        const char* nm=sec.tabs[i].name;
        ImVec2 nts=ImGui::CalcTextSize(nm);
        d->AddText(tc-nts*0.5f,
            Mix(Mix(C_TXT,C_TXT_H,h),U(cfg.col.tab_text),a), nm);
        ImGui::PopFont();
        ImGui::PopID();
    }

    // Thin line under tabs
    d->AddLine(o+S2(98,94), o+S2(622,94), C_LINE, 1.f);

    // ── Content panel ─────────────────────────────────────────────────────────
    ImGui::SetCursorScreenPos(o+S2(98,96));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   {0,0});
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0,0,0,0));
    ImGui::BeginChild("##panel",S2(622,360),false,
        ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoBackground);
    PF(fMed);
    DrawRows(sec.tabs[g_tab[g_cur]].rows, g_scroll[g_cur][g_tab[g_cur]]);
    ImGui::PopFont();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    ColorsPopup(o);
    ImGui::End();
}

} // namespace Menu
