#pragma once
#include <cstdint>

namespace ff {
    // Scan /proc for Free Fire process, return PID or 0 if not found.
    // Call each frame (cheap — cached, re-checks every ~1 s).
    int32_t get_pid();

    // Package name we look for
    inline constexpr const char* PACKAGE = "com.dts.freefireth";
}
