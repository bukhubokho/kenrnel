#pragma once
namespace screenshot {
    // Capture current GL framebuffer → /sdcard/DCIM/Screenshots/claudepro_*.bmp
    // Returns true on success.
    bool take(int width, int height);
}
