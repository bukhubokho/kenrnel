#pragma once
#include <cstdint>

namespace volume_key {
    // Start background thread listening for volume double-press toggle.
    // Double-press = 2 KEY_VOLUMEUP or KEY_VOLUMEDOWN presses within window_ms.
    void init(uint32_t window_ms = 500);
    void shutdown();
}
