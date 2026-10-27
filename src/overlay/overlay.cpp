// External GPU overlay — SurfaceComposerClient, no JVM needed
// Symbols confirmed from device: Android 12/13 Redmi Note 13 Pro 5G (garnet)
#include "overlay.h"
#include <android/log.h>
#include <dlfcn.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

#define LOG_TAG "FFKRN"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// sp<T> on Android is just a single pointer field
struct sp_t { void* ptr; };

// AArch64: sp<T> has non-trivial destructor → returned via x8 (indirect result register),
// NOT x0. We need inline asm to call such functions correctly.
// call_x8_ret0: fn() → result in x8
static void call_x8_ret0(void* fn, sp_t* out) {
    asm volatile(
        "mov x8, %1\n\t"
        "blr %0\n\t"
        :
        : "r"(fn), "r"(out)
        : "x0","x1","x2","x3","x4","x5","x6","x7","x8",
          "x9","x10","x11","x12","x13","x14","x15","x16","x17","x30","memory"
    );
}
// call_x8_ret1: fn(x0) → result in x8
static void call_x8_ret1(void* fn, void* a0, sp_t* out) {
    asm volatile(
        "mov x0, %1\n\t"
        "mov x8, %2\n\t"
        "blr %0\n\t"
        :
        : "r"(fn), "r"(a0), "r"(out)
        : "x0","x1","x2","x3","x4","x5","x6","x7","x8",
          "x9","x10","x11","x12","x13","x14","x15","x16","x17","x30","memory"
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// CONFIRMED symbol from device grep:
//  createSurfaceChecked returns status_t (int32_t) — no hidden ABI return ptr
//
// _ZN7android21SurfaceComposerClient20createSurfaceCheckedE
//   RKNS_7String8E          const String8&   name
//   jji                     uint32, uint32, int32  (w, h, format)
//   PNS_2spINS_14SurfaceControlEEE   sp<SurfaceControl>* out
//   i                       int32  flags
//   RKNS4_INS_7IBinderEEE   const sp<IBinder>& parent
//   NS_3gui13LayerMetadataE gui::LayerMetadata (non-trivial → hidden ptr in ABI)
//   Pj                      uint32_t* outHint
//
// AArch64 AAPCS64 calling convention for createSurfaceChecked:
//   x0 = thiz
//   x1 = &name  (String8&)
//   x2 = w
//   x3 = h
//   x4 = format
//   x5 = outSurface (sp<SC>*)
//   x6 = flags
//   x7 = &parent (sp<IBinder>&) — reference, stays as-is
//   [sp+0] = &meta (non-trivial by-value → passed as ptr, goes on stack after x7 fills)
//   [sp+8] = outHint
// ─────────────────────────────────────────────────────────────────────────────

// typedef matching the actual AArch64 ABI layout described above
typedef int32_t (*fn_createChecked)(
    void*     thiz,        // x0
    void*     name,        // x1
    uint32_t  w,           // x2
    uint32_t  h,           // x3
    int32_t   format,      // x4
    void*     outSurface,  // x5  sp<SurfaceControl>*
    int32_t   flags,       // x6
    void*     parent,      // x7  const sp<IBinder>&
    void*     meta,        // stack[0]  LayerMetadata* (indirect)
    uint32_t* outHint      // stack[8]
);

// getSurface() const → sp<Surface>
// sp<Surface> is non-trivial → returned via x8 (indirect result register)
// Use inline asm wrapper below
typedef void (*fn_getSurf)(void* ret_sp, void* sc);

// Transaction methods — scalar / void returns, straightforward
typedef void    (*fn_TX_ctor    )(void* tx);
typedef void*   (*fn_TX_setLayer)(void* tx, void* sc, int32_t z);
typedef void*   (*fn_TX_show   )(void* tx, void* sc);
typedef int32_t (*fn_TX_apply  )(void* tx, bool sync);

// ProcessState
typedef void* (*fn_PS_self )();
typedef void  (*fn_PS_start)(void*);

// String8(const char*)
typedef void (*fn_S8_ctor)(void* s8, const char* str);
typedef void (*fn_S8_dtor)(void* s8);

// ─────────────────────────────────────────────────────────────────────────────

static void* g_libgui    = nullptr;
static void* g_libbinder = nullptr;

static void* try_sym(void* lib, const char* const* names, int n, const char* what) {
    for (int i = 0; i < n; i++) {
        void* s = dlsym(lib, names[i]);
        if (s) { LOGI("found %s [%d]", what, i); return s; }
    }
    LOGE("NOT FOUND: %s", what);
    return nullptr;
}

static bool init_binder() {
    g_libbinder = dlopen("libbinder.so", RTLD_NOW | RTLD_GLOBAL);
    if (!g_libbinder) { LOGE("libbinder.so: %s", dlerror()); return false; }
    const char* self_s[]  = { "_ZN7android12ProcessState4selfEv" };
    const char* start_s[] = { "_ZN7android12ProcessState14startThreadPoolEv" };
    auto ps_self  = (fn_PS_self )try_sym(g_libbinder, self_s,  1, "ProcessState::self");
    auto ps_start = (fn_PS_start)try_sym(g_libbinder, start_s, 1, "ProcessState::startThreadPool");
    if (ps_self && ps_start) { ps_start(ps_self()); LOGI("binder OK"); }
    return true;
}

static void get_screen_size(int& w, int& h) {
    w = 1080; h = 2400;
    FILE* f = fopen("/sys/class/graphics/fb0/virtual_size", "r");
    if (f) { fscanf(f, "%d,%d", &w, &h); fclose(f); return; }
    f = fopen("/sys/class/drm/card0-DSI-1/modes", "r");
    if (f) { fscanf(f, "%dx%d", &w, &h); fclose(f); }
}

bool overlay_create(OverlayContext& ctx) {
    get_screen_size(ctx.width, ctx.height);
    fprintf(stderr, "[OVL] screen %dx%d\n", ctx.width, ctx.height);

    if (!init_binder()) { fprintf(stderr, "[OVL] ERR: binder init failed\n"); return false; }
    fprintf(stderr, "[OVL] binder OK\n");

    dlopen("libui.so",   RTLD_NOW | RTLD_GLOBAL);
    dlopen("libutils.so",RTLD_NOW | RTLD_GLOBAL);
    g_libgui = dlopen("libgui.so", RTLD_NOW | RTLD_GLOBAL);
    if (!g_libgui) {
        fprintf(stderr, "[OVL] ERR: libgui.so: %s\n", dlerror());
        return false;
    }
    fprintf(stderr, "[OVL] libgui.so OK\n");

    // ── SurfaceComposerClient::getDefault() ────────────────────────────
    // getDefault() returns sp<SCC> via x8 (non-trivial destructor).
    // It's a static method that returns the already-connected global SCC.
    // Available since Android 11 (API 30).
    fprintf(stderr, "[OVL] uid=%d, getting SCC via getDefault()\n", (int)getuid());

    void* fn_gd = dlsym(g_libgui, "_ZN7android21SurfaceComposerClient10getDefaultEv");
    sp_t scc_sp = {nullptr};
    if (fn_gd) {
        call_x8_ret0(fn_gd, &scc_sp);
        fprintf(stderr, "[OVL] getDefault() scc=%p\n", scc_sp.ptr);
    }

    // Fallback: construct SCC then call onFirstRef() to trigger connection
    if (!scc_sp.ptr) {
        fprintf(stderr, "[OVL] getDefault not found, trying ctor+onFirstRef\n");
        const char* scc_ctor_s[] = {
            "_ZN7android21SurfaceComposerClientC1Ev",
            "_ZN7android21SurfaceComposerClientC2Ev",
        };
        auto scc_ctor = (fn_TX_ctor)try_sym(g_libgui, scc_ctor_s, 2, "SCC::ctor");
        if (!scc_ctor) { fprintf(stderr, "[OVL] ERR: SCC::ctor not found\n"); return false; }
        void* raw = calloc(1, 512);
        scc_ctor(raw);
        // onFirstRef() triggers the actual SurfaceFlinger connection
        typedef void (*fn_ofr)(void*);
        auto fn_ofr_ = (fn_ofr)dlsym(g_libgui, "_ZN7android21SurfaceComposerClient10onFirstRefEv");
        if (fn_ofr_) { fn_ofr_(raw); fprintf(stderr, "[OVL] onFirstRef called\n"); }
        scc_sp.ptr = raw;
    }

    void* scc = scc_sp.ptr;
    if (!scc) { fprintf(stderr, "[OVL] ERR: no SCC\n"); return false; }

    // initCheck() — should be 0 now
    typedef int32_t (*fn_initCheck)(void*);
    auto fn_ic = (fn_initCheck)dlsym(g_libgui, "_ZNK7android21SurfaceComposerClient9initCheckEv");
    if (fn_ic) {
        int32_t ic = fn_ic(scc);
        fprintf(stderr, "[OVL] SCC initCheck=%d%s\n", ic,
                ic == 0 ? " OK" : ic == -19 ? " ENODEV(not connected)" : " ERR");
        if (ic != 0) { return false; }
    }

    // ── String8 ─────────────────────────────────────────────────────────
    void* libutils = dlopen("libutils.so", RTLD_NOLOAD);
    const char* s8_ctor_s[] = {
        "_ZN7android7String8C1EPKc",
        "_ZN7android7String8C2EPKc",
    };
    auto s8_ctor = (fn_S8_ctor)(try_sym(g_libgui,    s8_ctor_s, 2, "String8::ctor") ?:
                                try_sym(libutils,     s8_ctor_s, 2, "String8::ctor(utils)"));
    char name_buf[64] = {};
    if (s8_ctor) s8_ctor(name_buf, "KernelOverlay");
    fprintf(stderr, "[OVL] String8 %s\n", s8_ctor ? "OK" : "SKIP(raw)");

    // ── createSurfaceChecked (confirmed on device) ───────────────────────
    const char* create_s[] = {
        // CONFIRMED on device
        "_ZN7android21SurfaceComposerClient20createSurfaceCheckedERKNS_7String8EjjiPNS_2spINS_14SurfaceControlEEEiRKNS4_INS_7IBinderEEENS_3gui13LayerMetadataEPj",
        // fallback without gui:: namespace (older Android)
        "_ZN7android21SurfaceComposerClient20createSurfaceCheckedERKNS_7String8EjjiPNS_2spINS_14SurfaceControlEEEiRKNS4_INS_7IBinderEEENS_13LayerMetadataEPj",
    };
    auto create_fn = (fn_createChecked)try_sym(g_libgui, create_s, 2, "createSurfaceChecked");
    if (!create_fn) { fprintf(stderr, "[OVL] ERR: createSurfaceChecked not found\n"); return false; }
    fprintf(stderr, "[OVL] createSurfaceChecked found\n");

    // sp<SurfaceControl> result — just 8 bytes (single pointer)
    sp_t sc_sp = {nullptr};
    // sp<IBinder> null parent — just 8 bytes zeroed
    sp_t parent_sp = {nullptr};
    // gui::LayerMetadata — contains std::unordered_map which MUST be constructed, not zeroed
    char meta_buf[512] = {};
    {
        typedef void (*fn_LM_ctor)(void*);
        void* fn_lm = dlsym(g_libgui, "_ZN7android3gui13LayerMetadataC1Ev");
        if (!fn_lm) fn_lm = dlsym(g_libgui, "_ZN7android3gui13LayerMetadataC2Ev");
        // also try without gui:: namespace (older Android)
        if (!fn_lm) fn_lm = dlsym(g_libgui, "_ZN7android13LayerMetadataC1Ev");
        if (!fn_lm) fn_lm = dlsym(g_libgui, "_ZN7android13LayerMetadataC2Ev");
        if (fn_lm) {
            ((fn_LM_ctor)fn_lm)(meta_buf);
            fprintf(stderr, "[OVL] LayerMetadata ctor OK\n");
        } else {
            // fallback: unordered_map zero-init trick — set bucket_count=0, load=0
            // std::unordered_map on libc++ has 56-byte layout; zeroing is not valid
            // but with no ctor found we have to try
            fprintf(stderr, "[OVL] WARN: LayerMetadata ctor not found — zeroed (may crash)\n");
        }
    }
    uint32_t outHint = 0;

    fprintf(stderr, "[OVL] calling createSurfaceChecked...\n");
    // PixelFormat RGBA_8888 = 1
    int32_t status = create_fn(
        scc,
        (void*)name_buf,
        (uint32_t)ctx.width,
        (uint32_t)ctx.height,
        1,              // RGBA_8888
        &sc_sp,         // out sp<SurfaceControl>
        0,              // flags
        &parent_sp,     // null parent
        meta_buf,       // LayerMetadata (passed as indirect ptr on stack)
        &outHint
    );
    fprintf(stderr, "[OVL] createSurfaceChecked status=%d sc=%p\n", status, sc_sp.ptr);
    if (status != 0 || !sc_sp.ptr) {
        fprintf(stderr, "[OVL] ERR: createSurfaceChecked failed status=%d\n", status);
        return false;
    }
    void* sc = sc_sp.ptr;

    // ── Transaction ─────────────────────────────────────────────────────
    const char* tx_ctor_s[] = {
        "_ZN7android21SurfaceComposerClient11TransactionC1Ev",
        "_ZN7android21SurfaceComposerClient11TransactionC2Ev",
    };
    const char* tx_setlayer_s[] = {
        "_ZN7android21SurfaceComposerClient11Transaction8setLayerEPNS_14SurfaceControlEi",
        "_ZN7android21SurfaceComposerClient11Transaction8setLayerEPKNS_14SurfaceControlEi",
        "_ZN7android21SurfaceComposerClient11Transaction8setLayerERKNS_2spINS_14SurfaceControlEEEi",
    };
    const char* tx_show_s[] = {
        "_ZN7android21SurfaceComposerClient11Transaction4showEPNS_14SurfaceControlE",
        "_ZN7android21SurfaceComposerClient11Transaction4showEPKNS_14SurfaceControlE",
        "_ZN7android21SurfaceComposerClient11Transaction4showERKNS_2spINS_14SurfaceControlEEE",
    };
    const char* tx_apply_s[] = {
        "_ZN7android21SurfaceComposerClient11Transaction5applyEb",
        "_ZN7android21SurfaceComposerClient11Transaction5applyEv",
    };

    auto tx_ctor     = (fn_TX_ctor    )try_sym(g_libgui, tx_ctor_s,     2, "TX::ctor");
    auto tx_setlayer = (fn_TX_setLayer)try_sym(g_libgui, tx_setlayer_s, 3, "TX::setLayer");
    auto tx_show     = (fn_TX_show    )try_sym(g_libgui, tx_show_s,     3, "TX::show");
    auto tx_apply    = (fn_TX_apply   )try_sym(g_libgui, tx_apply_s,    2, "TX::apply");

    // Heap-allocate Transaction (can be >2KB in Android 12 with vectors)
    void* tx_buf = calloc(1, 4096);
    if (tx_ctor) tx_ctor(tx_buf);
    fprintf(stderr, "[OVL] TX ctor OK, setLayer=%p show=%p apply=%p\n",
            (void*)tx_setlayer, (void*)tx_show, (void*)tx_apply);

    // Android 12: setLayer/show take const sp<SurfaceControl>& — pass &sc_sp (pointer to sp wrapper)
    // Android <10: they take SurfaceControl* — pass sc directly
    // Detect by symbol: if ERKNS_2sp (sp& variant) was found → pass &sc_sp
    // We try sp& first in our list (index 2), raw ptr is index 0/1
    // Safe approach: always pass &sc_sp — for sp& version this is correct;
    // for raw ptr version sc_sp.ptr == sc so &sc_sp points to the right pointer value.
    // The function reads *(&sc_sp) = sc_sp.ptr = sc — same value, one level of indirection.
    void* sc_arg = &sc_sp;  // works for both const sp<SC>& and SC* (sc_sp.ptr == sc)
    if (tx_setlayer) tx_setlayer(tx_buf, sc_arg, 0x7fffffff);
    fprintf(stderr, "[OVL] setLayer OK\n");
    if (tx_show)     tx_show    (tx_buf, sc_arg);
    fprintf(stderr, "[OVL] show OK\n");
    if (tx_apply)    tx_apply   (tx_buf, false);
    fprintf(stderr, "[OVL] Transaction applied\n");
    free(tx_buf);

    // ── getSurface() → ANativeWindow ────────────────────────────────────
    // getSurface returns sp<Surface> via x8 (indirect result register on ARM64)
    // Surface inherits ANativeWindow — ANW magic = 0x00574e41 at offset 0
    const char* getsurf_s[] = {
        "_ZNK7android14SurfaceControl9getSurfaceEv",
        "_ZN7android14SurfaceControl9getSurfaceEv",
    };
    auto get_surf_fn = (fn_getSurf)try_sym(g_libgui, getsurf_s, 2, "SC::getSurface");

    sp_t surf_sp = {nullptr};
    if (get_surf_fn) {
        // getSurface(this) returns sp<Surface> via x8 — use inline asm helper
        call_x8_ret1((void*)get_surf_fn, sc, &surf_sp);
        fprintf(stderr, "[OVL] getSurface ptr=%p\n", surf_sp.ptr);
    } else {
        fprintf(stderr, "[OVL] getSurface: fn not found, scanning SC\n");
    }

    if (surf_sp.ptr) {
        uint32_t magic = *(uint32_t*)surf_sp.ptr;
        fprintf(stderr, "[OVL] getSurface magic=0x%08x\n", magic);
        if (magic == 0x00574e41 || magic == 0xDEF10001) {
            ctx.window = (ANativeWindow*)surf_sp.ptr;
            ANativeWindow_acquire(ctx.window);
            fprintf(stderr, "[OVL] ANativeWindow OK via getSurface\n");
            return true;
        }
        fprintf(stderr, "[OVL] bad magic, scanning SC fields\n");
    }

    // ── Fallback: scan SurfaceControl fields for ANativeWindow ──────────
    fprintf(stderr, "[OVL] scanning SurfaceControl @ %p\n", sc);
    uint64_t* sc_fields = (uint64_t*)sc;
    for (int off = 0; off < 32; off++) {
        void* candidate = (void*)sc_fields[off];
        if (!candidate || ((uintptr_t)candidate & 0xfff)) continue;
        uint32_t magic = *(uint32_t*)candidate;
        if (magic == 0x00574e41) {
            ctx.window = (ANativeWindow*)candidate;
            ANativeWindow_acquire(ctx.window);
            fprintf(stderr, "[OVL] ANativeWindow found at SC[%d]=%p\n", off, ctx.window);
            return true;
        }
    }

    fprintf(stderr, "[OVL] ERR: could not find ANativeWindow\n");
    return false;
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
    EGLConfig cfg; EGLint ncfg = 0;
    eglChooseConfig(ctx.display, attribs, &cfg, 1, &ncfg);
    if (!ncfg) { LOGE("eglChooseConfig failed"); return false; }

    ctx.surface = eglCreateWindowSurface(ctx.display, cfg, ctx.window, nullptr);
    if (ctx.surface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface: 0x%x", eglGetError());
        return false;
    }
    const EGLint ctx_a[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    ctx.context = eglCreateContext(ctx.display, cfg, EGL_NO_CONTEXT, ctx_a);
    if (!eglMakeCurrent(ctx.display, ctx.surface, ctx.surface, ctx.context)) {
        LOGE("eglMakeCurrent: 0x%x", eglGetError());
        return false;
    }
    ctx.ready = true;
    LOGI("EGL OK");
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
