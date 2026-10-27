#include "driver/driver.h"
#include "ff/memory.h"
#include "overlay/overlay.h"
#include "esp/esp.h"
#include "menu/menu.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_android.h"
#include "imgui/imgui_impl_opengl3.h"

#include <android/log.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

#define LOG_TAG "FFKRN"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Read key.txt from same directory as binary (or /data/local/tmp/key.txt)
static std::string load_key() {
    const char* paths[] = {
        "/data/local/tmp/key.txt",
        "/sdcard/key.txt",
        "key.txt",
    };
    char buf[512]{};
    for (auto p : paths) {
        int f = open(p, O_RDONLY);
        if (f < 0) continue;
        ssize_t n = read(f, buf, sizeof(buf)-1);
        close(f);
        if (n > 0) {
            // trim whitespace
            buf[n] = 0;
            while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r' || buf[n-1] == ' '))
                buf[--n] = 0;
            return std::string(buf);
        }
    }
    return "";
}

// Basic key validation — check length and prefix (extend with HTTPS verify if needed)
static bool validate_key(const std::string& key) {
    if (key.size() < 8) return false;
    // Any non-empty key passes local check; add HTTPS call here to replicate DVLXKAI behavior
    return true;
}

// Check if RTkernel driver is loaded (BY* device exists)
static bool driver_loaded() {
    DIR* d = opendir("/dev");
    if (!d) return false;
    struct dirent* ent;
    while ((ent = readdir(d))) {
        if (strncmp(ent->d_name, "BY", 2) == 0 && strlen(ent->d_name) > 2) {
            closedir(d);
            return true;
        }
    }
    closedir(d);
    return false;
}


static Camera read_camera(const FFState& st) {
    Camera cam{};
    cam.screen_w = FF::SCREEN_W;
    cam.screen_h = FF::SCREEN_H;
    // TODO: fill from game memory once offsets known
    cam.proj[0]=1; cam.proj[5]=1; cam.proj[10]=1; cam.proj[15]=1;
    cam.view[0]=1; cam.view[5]=1; cam.view[10]=1; cam.view[15]=1;
    return cam;
}

int main() {
    LOGI("starting");

    std::string key = load_key();
    LOGI("starting, key=%s", key.empty() ? "none" : "ok");

    // 2. Driver check
    if (!driver_loaded()) {
        fprintf(stderr, "[KERNEL] ERR: Driver /dev/BY* not found. Run 5.10.sh first.\n");
        return 1;
    }
    fprintf(stderr, "[KERNEL] Driver OK\n");

    // 3. Init FF memory
    FFState ff{};
    if (!ff_init(ff)) {
        fprintf(stderr, "[KERNEL] WARN: FF process not found yet, will retry\n");
    } else {
        fprintf(stderr, "[KERNEL] FF pid=%u il2cpp=0x%lx\n", ff.ff_pid, ff.il2cpp);
    }

    // 4. Create overlay window + EGL
    fprintf(stderr, "[KERNEL] Creating overlay...\n");
    OverlayContext overlay{};
    if (!overlay_create(overlay)) {
        fprintf(stderr, "[KERNEL] ERR: overlay_create failed\n");
        return 1;
    }
    fprintf(stderr, "[KERNEL] Overlay window OK %dx%d\n", overlay.width, overlay.height);
    if (!overlay_init_egl(overlay)) {
        fprintf(stderr, "[KERNEL] ERR: EGL init failed\n");
        return 1;
    }
    fprintf(stderr, "[KERNEL] EGL OK\n");

    FF::SCREEN_W = overlay.width;
    FF::SCREEN_H = overlay.height;

    // 5. ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = {(float)overlay.width, (float)overlay.height};
    io.IniFilename = nullptr;

    // Load a nice font — embedded Inter/Roboto would go here
    // For now use default at 16px, scale up for phone DPI
    io.FontGlobalScale = (float)overlay.height / 1080.f * 1.5f;

    ImGui_ImplAndroid_Init(nullptr);
    ImGui_ImplOpenGL3_Init("#version 100");

    MenuState    menu{};
    ESPConfig    esp_cfg{};
    bool         keep_running = true;

    menu.visible = true;  // show immediately on launch

    LOGI("main loop start %dx%d", overlay.width, overlay.height);

    while (keep_running) {
        // Try to find FF in background — don't block rendering
        if (!ff.ready) ff_init(ff);

        // Read entities only when FF is ready
        std::vector<Entity> entities;
        Camera cam{};
        cam.screen_w = FF::SCREEN_W;
        cam.screen_h = FF::SCREEN_H;
        cam.proj[0]=1; cam.proj[5]=1; cam.proj[10]=1; cam.proj[15]=1;
        cam.view[0]=1; cam.view[5]=1; cam.view[10]=1; cam.view[15]=1;
        if (ff.ready) {
            entities = read_entities(ff);
            cam = read_camera(ff);
        }
        menu_apply(menu, esp_cfg);

        // Render frame
        overlay_begin_frame(overlay);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame();
        ImGui::NewFrame();

        // ESP layer (always drawn)
        esp_draw(entities, cam, esp_cfg);

        // Menu (toggle with key/touch)
        if (menu.visible) {
            keep_running = menu_draw(menu, esp_cfg, key.c_str(), ff.ready);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        overlay_end_frame(overlay);

        // ~60fps
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();
    overlay_destroy(overlay);
    rt_close(ff.drv_fd);

    return 0;
}

// Screen dimensions storage
namespace FF {
    int SCREEN_W = 1080;
    int SCREEN_H = 2400;
}
