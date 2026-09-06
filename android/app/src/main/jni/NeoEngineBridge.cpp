#include <jni.h>
#include <string>
#include <android/log.h>
#include "Source/NeoEngine/Platform/Android/AndroidPlatform.h"

#define LOG_TAG "NeoEngineBridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C" {

JNIEXPORT void JNICALL
Java_com_fauzanengine_NeoEngineActivity_nativeInit(JNIEnv* env, jobject obj) {
    LOGI("Native Engine Initialized via JNI Bridge");
    NeoEngine::AndroidPlatform::Get().Init();
}

JNIEXPORT void JNICALL
Java_com_fauzanengine_NeoEngineActivity_nativeOnSurfaceCreated(JNIEnv* env, jobject obj, jobject surface) {
    LOGI("Native Surface Created");
}

JNIEXPORT void JNICALL
Java_com_fauzanengine_NeoEngineActivity_nativeOnSurfaceChanged(JNIEnv* env, jobject obj, jint width, jint height) {
    LOGI("Native Surface Changed: %dx%d", width, height);
    NeoEngine::AndroidPlatform::Get().SetDisplayMetrics(width, height, 320.0f);
}

JNIEXPORT void JNICALL
Java_com_fauzanengine_NeoEngineActivity_nativeOnSurfaceDestroyed(JNIEnv* env, jobject obj) {
    LOGI("Native Surface Destroyed");
}

JNIEXPORT void JNICALL
Java_com_fauzanengine_NeoEngineActivity_nativeTick(JNIEnv* env, jobject obj, jfloat deltaTime) {
    NeoEngine::AndroidPlatform::Get().PumpEvents();
}

JNIEXPORT void JNICALL
Java_com_fauzanengine_NeoEngineActivity_nativeInjectTouchEvent(JNIEnv* env, jobject obj, jint action, jfloat x, jfloat y) {
    NeoEngine::AndroidInputMotionEvent ev{action, x, y, 0, static_cast<uint64_t>(time(nullptr))};
    NeoEngine::AndroidPlatform::Get().InjectMotionEvent(ev);
}

}
