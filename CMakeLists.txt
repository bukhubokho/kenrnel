cmake_minimum_required(VERSION 3.22)
project(ffkernel)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ImGui sources (place imgui headers/sources in src/imgui/)
set(IMGUI_SOURCES
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui_impl_android.cpp
    src/imgui/imgui_impl_opengl3.cpp
)

set(SOURCES
    src/main.cpp
    src/driver/driver.cpp
    src/overlay/overlay.cpp
    src/esp/esp.cpp
    src/menu/menu.cpp
    ${IMGUI_SOURCES}
)

add_executable(ffkernel ${SOURCES})

target_include_directories(ffkernel PRIVATE
    src
    src/imgui
)

# Android system libraries
target_link_libraries(ffkernel
    log
    android
    EGL
    GLESv2
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
