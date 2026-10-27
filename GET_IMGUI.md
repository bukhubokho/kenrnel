# Get ImGui Source

The build needs ImGui 1.90.x source files in `src/imgui/`.
`build.bat` downloads them automatically. Manual steps:

1. Download: https://github.com/ocornut/imgui/releases/tag/v1.90.9
2. Extract these files into `src/imgui/`:
   - `imgui.h`, `imgui.cpp`
   - `imgui_internal.h`
   - `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`
   - `imconfig.h`, `imstb_*.h`
   - `backends/imgui_impl_android.h`, `backends/imgui_impl_android.cpp`
   - `backends/imgui_impl_opengl3.h`, `backends/imgui_impl_opengl3.cpp`
   - `backends/imgui_impl_opengl3_loader.h`

3. Delete the stub files already in `src/imgui/`:
   - `imgui_impl_android.h`, `imgui_impl_android.cpp`
   - `imgui_impl_opengl3.h`
   (the real ones from the ImGui repo replace them)

`build.bat` does steps 1-3 automatically on first run.
