#pragma once
#include "ff_read.hpp"
#include "../ui/menu_inflex.h"
#include "imgui.h"

namespace esp {

bool update_camera(int drv_fd, uint32_t pid, uint64_t il2cpp_base);
void draw(ImDrawList* dl, const ff::PlayerInfo* players, int count,
          const Menu::Settings& cfg, float sw, float sh);

extern bool g_cam_ok;

} // namespace esp
