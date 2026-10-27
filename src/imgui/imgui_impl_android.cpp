// ImGui Android backend — minimal impl for external overlay
// For full touch support, replace with the official imgui/backends/imgui_impl_android.cpp
#include "imgui.h"
#include "imgui_impl_android.h"
#include <android/input.h>
#include <chrono>

static std::chrono::time_point<std::chrono::steady_clock> g_time;

bool ImGui_ImplAndroid_Init(struct ANativeWindow*) {
    g_time = std::chrono::steady_clock::now();
    return true;
}

void ImGui_ImplAndroid_Shutdown() {}

void ImGui_ImplAndroid_NewFrame() {
    ImGuiIO& io = ImGui::GetIO();
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - g_time).count();
    g_time   = now;
    io.DeltaTime = dt > 0.f ? dt : 1.f/60.f;
}

int32_t ImGui_ImplAndroid_HandleInputEvent(const AInputEvent* event) {
    ImGuiIO& io = ImGui::GetIO();
    int32_t type = AInputEvent_getType(event);

    if (type == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        io.AddMousePosEvent(x, y);
        if (action == AMOTION_EVENT_ACTION_DOWN) io.AddMouseButtonEvent(0, true);
        if (action == AMOTION_EVENT_ACTION_UP)   io.AddMouseButtonEvent(0, false);
        return 1;
    }
    return 0;
}
