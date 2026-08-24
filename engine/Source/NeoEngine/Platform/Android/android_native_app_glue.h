#ifndef _ANDROID_NATIVE_APP_GLUE_H
#define _ANDROID_NATIVE_APP_GLUE_H

#include <poll.h>
#include <pthread.h>
#include <android/native_window.h>
#include <android/input.h>
#include <android/looper.h>

struct android_app;

struct android_poll_source {
    int32_t id;
    struct android_app* app;
    void (*process)(struct android_app* app, struct android_poll_source* source);
};

struct android_app {
    void* userData;
    void (*onAppCmd)(struct android_app* app, int32_t cmd);
    int32_t (*onInputEvent)(struct android_app* app, AInputEvent* event);
    ANativeWindow* window;
    int destroyRequested;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    struct android_poll_source cmdPollSource;
};

enum {
    APP_CMD_INIT_WINDOW = 1,
    APP_CMD_TERM_WINDOW = 2,
};
#endif
