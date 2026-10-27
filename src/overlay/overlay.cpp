// External GPU overlay using SurfaceComposerClient (no JVM required)
// Connects directly to SurfaceFlinger via Binder — same approach as bootanimation
#include "overlay.h"
#include <android/log.h>
#include <dlfcn.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>

#define LOG_TAG "FFKRN"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ─── sp<T> layout: just a pointer + refcount ───────────────────────────────
// Android's sp<> stores the pointer at offset 0
struct sp_t { void* ptr; };

// ─── Function pointer types ────────────────────────────────────────────────
typedef void*   (*fn_PS_self)();
typedef void    (*fn_PS_start)(void*);

// SurfaceComposerClient constructor / destructor
typedef void    (*fn_SCC_ctor)(void*);
typedef void    (*fn_SCC_dtor)(void*);

// String8 ctor/dtor
typedef void    (*fn_S8_ctor)(void*, const char*);
typedef void    (*fn_S8_dtor)(void*);

// createSurface returns sp<SurfaceControl> (= pointer in first 8 bytes of sp_t)
// Variant A: Android 10/11
typedef sp_t    (*fn_create_A)(void* thiz, void* name, uint32_t w, uint32_t h,
                               int32_t fmt, uint32_t flags, void* parent,
                               void* meta, uint32_t* outHint);
// Variant B: Android 12
typedef sp_t    (*fn_create_B)(void* thiz, void* name, int32_t w, int32_t h,
                               int32_t fmt, uint32_t flags, void* parent,
                               void* meta, uint32_t* outHint);

// SurfaceControl::getSurface() → sp<Surface> (= ANativeWindow* at ptr[0])
typedef sp_t    (*fn_SC_getSurf)(void* sc);

// Transaction methods (Android 12: take sp<SurfaceControl>* or raw ptr)
typedef void*   (*fn_TX_setLayer)(void* tx, void* sc, int32_t z);
typedef void*   (*fn_TX_show   )(void* tx, void* sc);
typedef void*   (*fn_TX_setAlpha)(void* tx, void* sc, float a);
typedef void*   (*fn_TX_setFill)(void* tx, void* sc, uint32_t f, uint32_t m);
typedef int32_t (*fn_TX_apply )(void* tx, bool sync);

// ─── Globals ──────────────────────────────────────────────────────────────
static void* g_libgui    = nullptr;
static void* g_libbinder = nullptr;
static void* g_libui     = nullptr;

// Try several mangled name variants for a symbol
static void* sym(void* lib, const char** names, int n) {
    for (int i = 0; i < n; i++) {
        void* s = dlsym(lib, names[i]);
        if (s) return s;
    }
    return nullptr;
}

static bool init_binder() {
    g_libbinder = dlopen("libbinder.so", RTLD_NOW | RTLD_GLOBAL);
    if (!g_libbinder) { LOGE("libbinder.so: %s", dlerror()); return false; }

    const char* self_names[] = {
        "_ZN7android12ProcessState4selfEv",
    };
    const char* start_names[] = {
        "_ZN7android12ProcessState14startThreadPoolEv",
    };
    auto ps_self  = (fn_PS_self) sym(g_libbinder, self_names,  1);
    auto ps_start = (fn_PS_start)sym(g_libbinder, start_names, 1);
    if (ps_self && ps_start) {
        void* ps = ps_self();
        ps_start(ps);
        LOGI("binder thread pool started");
    }
    return true;
}

// Screen size from fb0 or /sys
static void get_screen_size(int& w, int& h) {
    w = 1080; h = 2400; // safe default for Redmi Note 13 Pro 5G

    // Try reading from /sys/class/graphics/fb0/virtual_size
    FILE* f = fopen("/sys/class/graphics/fb0/virtual_size", "r");
    if (f) {
        int tw = 0, th = 0;
        if (fscanf(f, "%d,%d", &tw, &th) == 2 && tw > 0 && th > 0) {
            w = tw; h = th;
        }
        fclose(f);
        return;
    }
    // Try /sys/class/drm/card0-DSI-1/modes
    f = fopen("/sys/class/drm/card0-DSI-1/modes", "r");
    if (f) {
        int tw = 0, th = 0;
        fscanf(f, "%dx%d", &tw, &th);
        if (tw > 0 && th > 0) { w = tw; h = th; }
        fclose(f);
    }
}

bool overlay_create(OverlayContext& ctx) {
    get_screen_size(ctx.width, ctx.height);
    LOGI("screen %dx%d", ctx.width, ctx.height);

    if (!init_binder()) return false;

    g_libui  = dlopen("libui.so",   RTLD_NOW | RTLD_GLOBAL);
    g_libgui = dlopen("libgui.so",  RTLD_NOW | RTLD_GLOBAL);
    if (!g_libgui) { LOGE("libgui.so: %s", dlerror()); return false; }

    // ── SurfaceComposerClient constructor ───────────────────────────────
    const char* scc_ctor_names[] = {
        "_ZN7android21SurfaceComposerClientC1Ev",
        "_ZN7android21SurfaceComposerClientC2Ev",
    };
    auto scc_ctor = (fn_SCC_ctor)sym(g_libgui, scc_ctor_names, 2);
    if (!scc_ctor) { LOGE("SCC ctor not found"); return false; }

    // Allocate SCC on heap (typical size ~256 bytes on Android 12)
    void* scc = calloc(1, 512);
    scc_ctor(scc);
    LOGI("SurfaceComposerClient created");

    // ── String8("KernelOverlay") ────────────────────────────────────────
    const char* s8_ctor_names[] = {
        "_ZN7android7String8C1EPKc",
        "_ZN7android7String8C2EPKc",
    };
    auto s8_ctor = (fn_S8_ctor)sym(g_libgui, s8_ctor_names, 2);
    if (!s8_ctor) s8_ctor = (fn_S8_ctor)sym(g_libui, s8_ctor_names, 2);

    // String8 layout: just a const char* mString at offset 0
    char name_buf[64] = {};
    if (s8_ctor) s8_ctor(name_buf, "KernelOverlay");

    // ── createSurface ───────────────────────────────────────────────────
    const char* create_names[] = {
        // Android 12 (most common)
        "_ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8EjjijPNS_14SurfaceControlENS_13LayerMetadataEPj",
        // Android 11
        "_ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8EjjijPKNS_14SurfaceControlENS_13LayerMetadataEPj",
        // Android 10
        "_ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8EjjijPNS_14SurfaceControlEiPj",
        // Android 13
        "_ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8EjjijRKNS_2spINS_14SurfaceControlEEENS_13LayerMetadataEPj",
    };
    auto create_surf = (fn_create_A)sym(g_libgui, create_names, 4);
    if (!create_surf) { LOGE("createSurface not found"); return false; }

    // LayerMetadata = 64 bytes zeroed
    char meta[64] = {};
    // flags: ISurfaceComposerClient::eOpaque = 0x400 is NOT what we want
    // Use flags = 0 for a regular layer
    // PixelFormat RGBA_8888 = 1
    sp_t sc_sp = create_surf(scc, name_buf,
                             (uint32_t)ctx.width, (uint32_t)ctx.height,
                             1 /*RGBA_8888*/, 0 /*flags*/,
                             nullptr /*parent*/, meta, nullptr);
    void* sc = sc_sp.ptr;
    if (!sc) { LOGE("createSurface returned null"); return false; }
    LOGI("SurfaceControl = %p", sc);

    // ── Transaction: setLayer(INT_MAX) + show() ─────────────────────────
    const char* tx_setlayer_names[] = {
        "_ZN7android21SurfaceComposerClient11Transaction8setLayerEPKNS_14SurfaceControlEi",
        "_ZN7android21SurfaceComposerClient11Transaction8setLayerEPNS_14SurfaceControlEi",
        "_ZN7android21SurfaceComposerClient11Transaction8setLayerERKNS_2spINS_14SurfaceControlEEEi",
    };
    const char* tx_show_names[] = {
        "_ZN7android21SurfaceComposerClient11Transaction4showEPKNS_14SurfaceControlE",
        "_ZN7android21SurfaceComposerClient11Transaction4showEPNS_14SurfaceControlE",
        "_ZN7android21SurfaceComposerClient11Transaction4showERKNS_2spINS_14SurfaceControlEEE",
    };
    const char* tx_apply_names[] = {
        "_ZN7android21SurfaceComposerClient11Transaction5applyEb",
        "_ZN7android21SurfaceComposerClient11Transaction5applyEv",
    };

    auto tx_setlayer = (fn_TX_setLayer)sym(g_libgui, tx_setlayer_names, 3);
    auto tx_show     = (fn_TX_show    )sym(g_libgui, tx_show_names,     3);
    auto tx_apply    = (fn_TX_apply   )sym(g_libgui, tx_apply_names,    2);

    // Transaction is stack-allocated (~512 bytes typical)
    char tx_buf[1024] = {};
    // Try to find Transaction ctor
    const char* tx_ctor_names[] = {
        "_ZN7android21SurfaceComposerClient11TransactionC1Ev",
        "_ZN7android21SurfaceComposerClient11TransactionC2Ev",
    };
    auto tx_ctor = (fn_SCC_ctor)sym(g_libgui, tx_ctor_names, 2);
    if (tx_ctor) tx_ctor(tx_buf);

    if (tx_setlayer) tx_setlayer(tx_buf, sc, 0x7fffffff); // INT_MAX layer
    if (tx_show)     tx_show    (tx_buf, sc);
    if (tx_apply)    tx_apply   (tx_buf, false);
    LOGI("Transaction applied");

    // ── getSurface() → ANativeWindow* ───────────────────────────────────
    const char* getsurf_names[] = {
        "_ZNK7android14SurfaceControl9getSurfaceEv",
        "_ZN7android14SurfaceControl9getSurfaceEv",
    };
    auto get_surf = (fn_SC_getSurf)sym(g_libgui, getsurf_names, 2);
    if (!get_surf) {
        // Fallback: Surface* is stored in SurfaceControl at a known offset
        // On Android 12, SurfaceControl holds sp<Surface> at offset 0x10
        // Try reading it directly
        void** sc_fields = (void**)sc;
        for (int off = 0; off < 16; off++) {
            void* candidate = sc_fields[off];
            if (!candidate) continue;
            // ANativeWindow has magic = 0x00574e41 at offset 0
            uint32_t magic = *(uint32_t*)candidate;
            if (magic == 0x00574e41) {
                ctx.window = (ANativeWindow*)candidate;
                LOGI("ANativeWindow found via scan at offset %d: %p", off, ctx.window);
                ANativeWindow_acquire(ctx.window);
                return true;
            }
        }
        LOGE("getSurface not found and ANW scan failed");
        return false;
    }

    sp_t surf_sp = get_surf(sc);
    ctx.window = (ANativeWindow*)surf_sp.ptr;
    if (!ctx.window) { LOGE("getSurface returned null"); return false; }

    // Retain a reference
    ANativeWindow_acquire(ctx.window);
    LOGI("ANativeWindow = %p", ctx.window);
    return true;
}

bool overlay_init_egl(OverlayContext& ctx) {
    ctx.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(ctx.display, nullptr, nullptr);

    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_ALPHA_SIZE,      8,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_DEPTH_SIZE,      0,
        EGL_NONE
    };
    EGLConfig cfg;
    EGLint    ncfg = 0;
    eglChooseConfig(ctx.display, attribs, &cfg, 1, &ncfg);
    if (!ncfg) { LOGE("eglChooseConfig failed"); return false; }

    ctx.surface = eglCreateWindowSurface(ctx.display, cfg, ctx.window, nullptr);
    if (ctx.surface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed: %x", eglGetError());
        return false;
    }

    const EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    ctx.context = eglCreateContext(ctx.display, cfg, EGL_NO_CONTEXT, ctx_attribs);
    if (!eglMakeCurrent(ctx.display, ctx.surface, ctx.surface, ctx.context)) {
        LOGE("eglMakeCurrent failed: %x", eglGetError());
        return false;
    }

    ctx.ready = true;
    LOGI("EGL ready");
    return true;
}

void overlay_begin_frame(const OverlayContext& ctx) {
    glViewport(0, 0, ctx.width, ctx.height);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void overlay_end_frame(const OverlayContext& ctx) {
    eglSwapBuffers(ctx.display, ctx.surface);
}

void overlay_destroy(OverlayContext& ctx) {
    if (ctx.display != EGL_NO_DISPLAY) {
        eglMakeCurrent(ctx.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (ctx.surface != EGL_NO_SURFACE) eglDestroySurface(ctx.display, ctx.surface);
        if (ctx.context != EGL_NO_CONTEXT) eglDestroyContext(ctx.display, ctx.context);
        eglTerminate(ctx.display);
    }
    if (ctx.window) ANativeWindow_release(ctx.window);
    ctx.ready = false;
}
