// antilose_fonts.cpp
// Loads fonts for the new Inflex menu (Menu::LoadFontsMemory) using the
// pixeloperator typeface that is already embedded in the project.
// Called once from ImGui_init() before the atlas is built.
#include "imgui.h"
#include "antilose_menu.h"
#include "menu_inflex.h"
#include "../fonts/antilose_font.h"
#include "../fonts/antilose_fontlog.h"

void AntiloseLoadFonts() {
    ImGuiIO& io = ImGui::GetIO();

    // Watermark / ESP shared fonts (Logo, FontLogo, Esp) ----------------
    // These are still needed by antilose_watermark.h via AntiloseBindFonts().
    // We just register them here; AntiloseBindFonts() aliases them to the
    // draw-layer fontBold / espFont at runtime.
    // (No explicit AddFont call needed — AntiloseBindFonts does the alias.)
    (void)io; // suppress unused warning if the block below is empty

    // Inflex menu fonts --------------------------------------------------
    // The new menu uses Inter; fall back to the embedded AntiLose/pixeloperator
    // typeface so the build stays self-contained without extra .ttf files.
    // Sizes match the originals from menu.h comments:
    //   fMed  = 12.6 px * scale
    //   fSemi = 11.5 px * scale
    //   fBold = 11.5 px * scale
    // We load all three from the same binary blob (they will differ only in
    // weight when real Inter TTFs are provided via Menu::LoadFonts()).
    Menu::LoadFontsMemory(
        (void*)Font, (int)sizeof(Font),   // medium
        (void*)Font, (int)sizeof(Font),   // semibold  (same blob — acceptable fallback)
        (void*)Font, (int)sizeof(Font)    // bold      (same blob)
    );
}
