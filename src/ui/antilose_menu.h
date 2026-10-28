#pragma once
#include "imgui.h"

// Shared fonts (bound each frame in AntiloseBindFonts)
extern ImFont* Logo;
extern ImFont* FontLogo;
extern ImFont* Esp;

// Overlay unload signal — main loop checks IsUnloadRequested() and exits.
void RequestUnload();
bool IsUnloadRequested();

// Called once from ImGui_init() before the atlas is built.
void AntiloseLoadFonts();

// Called once per frame; syncs project fonts into shared pointers.
void AntiloseBindFonts();

// Accent colour sync — call after Menu::Render() to keep watermark in sync.
void SyncAccentFromMenu();

// Cfg sync — mirrors Menu::cfg → cfg::esp / cfg::aim each frame.
void SyncMenuCfg();

// Radial glow for watermark (defined in menu_inflex.cpp or compat shim).
void DrawRadialGradientEsp(const ImVec2& center, float radius, ImColor col_in);

// Global accent colour used by watermark.
extern ImVec4 accentColor;
