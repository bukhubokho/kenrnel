#pragma once
// ImGui OpenGL ES 2.0 backend
// Full source from imgui/backends/imgui_impl_opengl3.cpp in ImGui repo

bool ImGui_ImplOpenGL3_Init(const char* glsl_version = "#version 100");
void ImGui_ImplOpenGL3_Shutdown();
void ImGui_ImplOpenGL3_NewFrame();
void ImGui_ImplOpenGL3_RenderDrawData(struct ImDrawData* draw_data);
