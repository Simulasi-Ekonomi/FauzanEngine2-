#include "android_native_app_glue.h"
#include "Core/Engine.h"
#include <android/log.h>

#define LOG_TAG "NeoEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

void handle_cmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            LOGI("Native Window Initialized");
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("Native Window Terminated");
            break;
    }
}

// Ini adalah entry point utama untuk Android
void android_main(struct android_app* app) {
    app->onAppCmd = handle_cmd;

    int ident;
    int events;
    struct android_poll_source* source;

    // Ganti ALooper_pollAll menjadi ALooper_pollOnce
    while (Engine::Get().IsRunning()) {
        // Jika engine jalan, timeout 0 (non-blocking), jika tidak, -1 (blocking)
        while ((ident = ALooper_pollOnce(0, NULL, &events, (void**)&source)) >= 0) {
            if (source != NULL) {
                source->process(app, source);
            }
            if (app->destroyRequested != 0) {
                Engine::Get().Stop();
                break;
            }
        }
        
        // Jalankan loop utama engine di sini jika diperlukan
    }
}
