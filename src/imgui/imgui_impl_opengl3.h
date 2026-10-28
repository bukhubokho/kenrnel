#pragma once
// ImGui OpenGL ES 2.0 backend
// Full source from imgui/backends/imgui_impl_opengl3.cpp in ImGui repo

bool ImGui_ImplOpenGL3_Init(const char* glsl_version = "#version 100");
void ImGui_ImplOpenGL3_Shutdown();
void ImGui_ImplOpenGL3_NewFrame();
void ImGui_ImplOpenGL3_RenderDrawData(struct ImDrawData* draw_data);
// Android overlay: call when device rotation changes to pre-rotate rendering
// (0=portrait, 1=90°CW, 2=180°, 3=90°CCW); nat_w/nat_h = portrait buffer dims
void ImGui_ImplOpenGL3_SetRotation(int rot, int nat_w = 0, int nat_h = 0);
