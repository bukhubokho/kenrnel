#include "overlay.h"
#include <android/log.h>
#include <dlfcn.h>
#include <jni.h>
#include <cstdio>
#include <cstring>

#define LOG_TAG "FFKRN"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Reach the JVM through the running zygote / app process
static JavaVM* get_jvm() {
    void* art = dlopen("libart.so", RTLD_NOLOAD);
    if (!art) return nullptr;
    auto fn = (jint(*)(JavaVM**, jsize, jsize*))dlsym(art, "JNI_GetCreatedJavaVMs");
    if (!fn) return nullptr;
    JavaVM* vm = nullptr;
    jsize   cnt = 0;
    fn(&vm, 1, &cnt);
    return cnt ? vm : nullptr;
}

bool overlay_create(OverlayContext& ctx) {
    JavaVM* vm = get_jvm();
    if (!vm) { LOGE("no JVM"); return false; }

    JNIEnv* env = nullptr;
    bool attached = false;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        vm->AttachCurrentThread(&env, nullptr);
        attached = true;
    }

    // WindowManager.LayoutParams
    jclass   lp_cls  = env->FindClass("android/view/WindowManager$LayoutParams");
    jmethodID lp_ctor = env->GetMethodID(lp_cls, "<init>", "(IIIII)V");
    // TYPE_APPLICATION_OVERLAY = 2038, FLAG_NOT_FOCUSABLE|FLAG_NOT_TOUCH_MODAL|FLAG_LAYOUT_IN_SCREEN
    constexpr int TYPE  = 2038;
    constexpr int FLAGS = 0x28; // NOT_FOCUSABLE | NOT_TOUCH_MODAL
    constexpr int FMT   = -3;   // PixelFormat.TRANSLUCENT

    // Get display metrics for screen size
    jclass   dm_cls  = env->FindClass("android/util/DisplayMetrics");
    jmethodID dm_ctor = env->GetMethodID(dm_cls, "<init>", "()V");
    jobject   dm_obj  = env->NewObject(dm_cls, dm_ctor);

    jclass   ctx_cls  = env->FindClass("android/app/ActivityThread");
    jmethodID cur_app = env->GetStaticMethodID(ctx_cls, "currentApplication",
                            "()Landroid/app/Application;");
    jobject   app     = env->CallStaticObjectMethod(ctx_cls, cur_app);

    jclass    app_cls = env->GetObjectClass(app);
    jmethodID get_res = env->GetMethodID(app_cls, "getResources",
                            "()Landroid/content/res/Resources;");
    jobject   res     = env->CallObjectMethod(app, get_res);

    jclass    res_cls = env->GetObjectClass(res);
    jmethodID get_dm  = env->GetMethodID(res_cls, "getDisplayMetrics",
                            "()Landroid/util/DisplayMetrics;");
    dm_obj = env->CallObjectMethod(res, get_dm);

    jfieldID fw = env->GetFieldID(dm_cls, "widthPixels",  "I");
    jfieldID fh = env->GetFieldID(dm_cls, "heightPixels", "I");
    ctx.width  = env->GetIntField(dm_obj, fw);
    ctx.height = env->GetIntField(dm_obj, fh);

    // Create SurfaceView via SurfaceHolder — easier: use SurfaceTexture path
    // Simplest reliable path: create a Surface from SurfaceTexture
    jclass   st_cls   = env->FindClass("android/graphics/SurfaceTexture");
    jmethodID st_ctor = env->GetMethodID(st_cls, "<init>", "(I)V");
    jobject   st_obj  = env->NewObject(st_cls, st_ctor, 0);

    jmethodID st_setsize = env->GetMethodID(st_cls, "setDefaultBufferSize", "(II)V");
    env->CallVoidMethod(st_obj, st_setsize, ctx.width, ctx.height);

    jclass   surf_cls   = env->FindClass("android/view/Surface");
    jmethodID surf_ctor = env->GetMethodID(surf_cls, "<init>",
                              "(Landroid/graphics/SurfaceTexture;)V");
    jobject  surf_obj   = env->NewObject(surf_cls, surf_ctor, st_obj);

    // ANativeWindow from Surface
    void* libandroid = dlopen("libandroid.so", RTLD_NOLOAD);
    auto  ANW_from   = (ANativeWindow*(*)(JNIEnv*, jobject))
                          dlsym(libandroid, "ANativeWindow_fromSurface");
    ctx.window = ANW_from(env, surf_obj);

    if (attached) vm->DetachCurrentThread();

    if (!ctx.window) { LOGE("no ANativeWindow"); return false; }
    LOGI("overlay window %dx%d", ctx.width, ctx.height);
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
    glClearColor(0.f, 0.f, 0.f, 0.f);   // fully transparent
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
