cmake_minimum_required(VERSION 3.22)
project(ffkernel)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Shared sources (no main, no backend) ────────────────────────────────────
set(COMMON_SOURCES
    src/draw/draw.cpp
    src/touch/Touch.cpp

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
    src/ff/ff_pid.cpp
    src/ff/ff_read.cpp
    src/ff/esp_draw.cpp
    src/ui/widgets/widgets.cpp
    src/protect/oxorany.cpp
    src/mem/reader.cpp
)

set(COMMON_INCLUDES src src/imgui src/ff)

set(COMMON_LINK_LIBS log android EGL GLESv3 dl z m)

set(COMMON_COMPILE_OPTS
    -O2
    -fvisibility=hidden
    -ffunction-sections
    -fdata-sections
    -fno-exceptions
    -fno-rtti
)

set(COMMON_LINK_OPTS
    -Wl,--gc-sections
    -Wl,--strip-all
)

# ── ffkernel — RTkernel driver backend ───────────────────────────────────────
add_executable(ffkernel
    src/main.cpp
    src/driver/driver.cpp
    ${COMMON_SOURCES}
)
target_include_directories(ffkernel PRIVATE ${COMMON_INCLUDES})
target_link_libraries(ffkernel ${COMMON_LINK_LIBS})
target_compile_options(ffkernel PRIVATE ${COMMON_COMPILE_OPTS})
target_link_options(ffkernel PRIVATE ${COMMON_LINK_OPTS})

# ── ffext — /proc/pid/mem external backend (no driver) ───────────────────────
add_executable(ffext
    src/main_ext.cpp
    ${COMMON_SOURCES}
)
target_include_directories(ffext PRIVATE ${COMMON_INCLUDES})
target_link_libraries(ffext ${COMMON_LINK_LIBS})
target_compile_options(ffext PRIVATE ${COMMON_COMPILE_OPTS} -DUSE_PROC_MEM)
target_link_options(ffext PRIVATE ${COMMON_LINK_OPTS})
