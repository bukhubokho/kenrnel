#pragma once
#include "ff_read.hpp"
#include "../ui/menu_inflex.h"
#include "imgui.h"

namespace esp {

// Read camera matrix from FF and cache it — call each frame before draw()
// Returns true if matrix is valid
bool update_camera(int drv_fd, uint32_t pid, uint64_t il2cpp_base);

// Draw ESP for all valid players onto dl (use GetBackgroundDrawList)
void draw(ImDrawList* dl, const ff::PlayerInfo* players, int count,
          const Menu::Settings& cfg, float sw, float sh);

} // namespace esp
