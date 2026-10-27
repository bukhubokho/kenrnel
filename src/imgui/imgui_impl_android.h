#pragma once
// ImGui Android backend stub
// Full implementation: imgui/backends/imgui_impl_android.cpp from ImGui repo

bool ImGui_ImplAndroid_Init(struct ANativeWindow* window);
void ImGui_ImplAndroid_Shutdown();
void ImGui_ImplAndroid_NewFrame();

// Handle touch input — call from your input loop
// Returns true if ImGui consumed the event
struct AInputEvent;
int32_t ImGui_ImplAndroid_HandleInputEvent(const AInputEvent* event);
