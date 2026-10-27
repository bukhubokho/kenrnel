#pragma once
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/native_window.h>
#include <cstdint>

struct OverlayContext {
    ANativeWindow*  window  = nullptr;
    EGLDisplay      display = EGL_NO_DISPLAY;
    EGLSurface      surface = EGL_NO_SURFACE;
    EGLContext      context = EGL_NO_CONTEXT;
    int             width   = 0;
    int             height  = 0;
    bool            ready   = false;
};

// Create TYPE_APPLICATION_OVERLAY window via Android SurfaceFlinger JNI
bool overlay_create(OverlayContext& ctx);

// Init EGL + GLES2 on ctx.window
bool overlay_init_egl(OverlayContext& ctx);

// Begin frame: clear, set viewport
void overlay_begin_frame(const OverlayContext& ctx);

// End frame: swap buffers
void overlay_end_frame(const OverlayContext& ctx);

void overlay_destroy(OverlayContext& ctx);
