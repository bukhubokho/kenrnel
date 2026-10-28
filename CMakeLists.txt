cmake_minimum_required(VERSION 3.22)
project(ffkernel)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/driver/driver.cpp
    src/draw/draw.cpp
    src/touch/Touch.cpp
    src/touch/volume_key.cpp
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui_impl_android.cpp
    src/imgui/imgui_impl_opengl3.cpp
    src/ui/antilose_compat.cpp
    src/ui/antilose_fonts.cpp
    src/ui/antilose_box_shadow.cpp
    src/ui/bar.cpp
    src/ui/menu.cpp
    src/ui/screenshot.cpp
    src/ui/menu_inflex.cpp
    src/ui/widgets/widgets.cpp
    src/protect/oxorany.cpp
)

add_executable(ffkernel ${SOURCES})

target_include_directories(ffkernel PRIVATE
    src
    src/imgui
)

# GLES3 — matches Wolsaw draw.cpp (#version 300 es)
# No IMGUI_IMPL_OPENGL_ES2 — we want the full GLES3 path

# Android system libraries
target_link_libraries(ffkernel
    log
    android
    EGL
    GLESv3
    dl
    z
    m
)

# Strip + optimize for size
target_compile_options(ffkernel PRIVATE
    -O2
    -fvisibility=hidden
    -ffunction-sections
    -fdata-sections
    -fno-exceptions
    -fno-rtti
)

target_link_options(ffkernel PRIVATE
    -Wl,--gc-sections
    -Wl,--strip-all
)
