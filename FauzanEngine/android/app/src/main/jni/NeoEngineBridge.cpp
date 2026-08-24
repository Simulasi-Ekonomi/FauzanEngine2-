// =========================================================
// NeoEngine JNI Bridge
// Bridges C++ NeoEngine core with Java/Kotlin Android layer
// =========================================================

#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>
#include <memory>
#include <mutex>

#define NEO_JNI_TAG "NeoEngine-JNI"
#define NEO_LOGI(...) __android_log_print(ANDROID_LOG_INFO, NEO_JNI_TAG, __VA_ARGS__)
#define NEO_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, NEO_JNI_TAG, __VA_ARGS__)
#define NEO_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, NEO_JNI_TAG, __VA_ARGS__)

// Forward declarations
class Engine;

namespace NeoJNI {

static JavaVM* g_JavaVM = nullptr;
static jobject g_Activity = nullptr;
static AAssetManager* g_AssetManager = nullptr;
static std::mutex g_Mutex;
static bool g_Initialized = false;

// Engine state
static bool g_EngineRunning = false;
static float g_DeltaTime = 0.0f;
static int g_FrameCount = 0;
static float g_FPS = 0.0f;

// Telemetry data for Aries agents
struct TelemetryData {
    float cpuTemp = 0.0f;
    float gpuTemp = 0.0f;
    float frameRate = 60.0f;
    float memoryUsageMB = 0.0f;
    float batteryLevel = 100.0f;
    int activeEntities = 0;
    int drawCalls = 0;
    int triangleCount = 0;
};

static TelemetryData g_Telemetry;

JavaVM* GetJavaVM() { return g_JavaVM; }
AAssetManager* GetAssetManager() { return g_AssetManager; }

JNIEnv* GetJNIEnv() {
    JNIEnv* env = nullptr;
    if (g_JavaVM) {
        int status = g_JavaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        if (status == JNI_EDETACHED) {
            g_JavaVM->AttachCurrentThread(&env, nullptr);
        }
    }
    return env;
}

} // namespace NeoJNI

// =========================================================
// JNI Lifecycle
// =========================================================

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    NeoJNI::g_JavaVM = vm;
    NEO_LOGI("JNI_OnLoad: NeoEngine native library loaded");

    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        NEO_LOGE("JNI_OnLoad: Failed to get JNI environment");
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* /*reserved*/) {
    NEO_LOGI("JNI_OnUnload: NeoEngine native library unloading");
    NeoJNI::g_JavaVM = nullptr;
}

// =========================================================
// Engine Core Bridge
// =========================================================

JNIEXPORT jboolean JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeInit(
    JNIEnv* env, jclass /*clazz*/,
    jobject activity, jobject assetManager,
    jint screenWidth, jint screenHeight) {

    std::lock_guard<std::mutex> lock(NeoJNI::g_Mutex);

    NEO_LOGI("nativeInit: Initializing NeoEngine (%dx%d)", screenWidth, screenHeight);

    NeoJNI::g_Activity = env->NewGlobalRef(activity);
    NeoJNI::g_AssetManager = AAssetManager_fromJava(env, assetManager);

    if (!NeoJNI::g_AssetManager) {
        NEO_LOGE("nativeInit: Failed to get AssetManager");
        env->DeleteGlobalRef(NeoJNI::g_Activity);
        NeoJNI::g_Activity = nullptr;
        return JNI_FALSE;
    }

    // Initialize engine subsystems
    NeoJNI::g_EngineRunning = true;
    NeoJNI::g_Initialized = true;
    NeoJNI::g_FrameCount = 0;

    NEO_LOGI("nativeInit: Engine initialized successfully");
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeShutdown(
    JNIEnv* env, jclass /*clazz*/) {

    std::lock_guard<std::mutex> lock(NeoJNI::g_Mutex);

    NEO_LOGI("nativeShutdown: Shutting down NeoEngine");
    NeoJNI::g_EngineRunning = false;
    NeoJNI::g_Initialized = false;

    if (NeoJNI::g_Activity) {
        env->DeleteGlobalRef(NeoJNI::g_Activity);
        NeoJNI::g_Activity = nullptr;
    }
}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeTick(
    JNIEnv* /*env*/, jclass /*clazz*/, jfloat deltaTime) {

    if (!NeoJNI::g_EngineRunning) return;

    NeoJNI::g_DeltaTime = deltaTime;
    NeoJNI::g_FrameCount++;

    // Calculate FPS
    static float fpsTimer = 0.0f;
    static int fpsFrames = 0;
    fpsTimer += deltaTime;
    fpsFrames++;
    if (fpsTimer >= 1.0f) {
        NeoJNI::g_FPS = static_cast<float>(fpsFrames) / fpsTimer;
        NeoJNI::g_Telemetry.frameRate = NeoJNI::g_FPS;
        fpsTimer = 0.0f;
        fpsFrames = 0;
    }
}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeRender(
    JNIEnv* /*env*/, jclass /*clazz*/) {

    if (!NeoJNI::g_EngineRunning) return;
    // Rendering is handled by the Vulkan/GLES render thread
}

// =========================================================
// Input Bridge
// =========================================================

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeTouchEvent(
    JNIEnv* /*env*/, jclass /*clazz*/,
    jint action, jfloat x, jfloat y, jint pointerId) {

    if (!NeoJNI::g_EngineRunning) return;

    NEO_LOGD("Touch event: action=%d pos=(%.1f, %.1f) pointer=%d",
             action, x, y, pointerId);
}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeKeyEvent(
    JNIEnv* /*env*/, jclass /*clazz*/,
    jint keyCode, jint action) {

    if (!NeoJNI::g_EngineRunning) return;

    NEO_LOGD("Key event: keyCode=%d action=%d", keyCode, action);
}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeSensorEvent(
    JNIEnv* /*env*/, jclass /*clazz*/,
    jint sensorType, jfloat x, jfloat y, jfloat z) {

    if (!NeoJNI::g_EngineRunning) return;
    // Accelerometer/Gyroscope data for motion controls
}

// =========================================================
// Telemetry Bridge (for Aries AI agents)
// =========================================================

JNIEXPORT jfloat JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeGetFPS(
    JNIEnv* /*env*/, jclass /*clazz*/) {
    return NeoJNI::g_FPS;
}

JNIEXPORT jfloat JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeGetCPUTemp(
    JNIEnv* /*env*/, jclass /*clazz*/) {
    return NeoJNI::g_Telemetry.cpuTemp;
}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeSetThrottleLevel(
    JNIEnv* /*env*/, jclass /*clazz*/, jint level) {

    NEO_LOGI(\"Throttle level set to %d by Aries agent\", level);
    // Adjust engine performance based on Aries AI decision
    // 0 = no throttle, 1 = light, 2 = medium, 3 = heavy
}

// =========================================================
// LiteRT/Gemma4 Integration Bridge
// =========================================================

JNIEXPORT jstring JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeSendGemmaPrompt(
    JNIEnv* env, jclass /*clazz*/, jstring prompt) {

    const char* nativePrompt = env->GetStringUTFChars(prompt, nullptr);
    std::string response;

    try {
        // Call the LiteRTManager's sendPrompt method via static method
        jclass bridgeClass = env->FindClass("com/neoengine/core/NeoEngineBridge");
        jmethodID sendPromptMethod = env->GetStaticMethodID(
            bridgeClass, 
            "sendPrompt", 
            "(Ljava/lang/String;)Ljava/lang/String;"
        );

        if (sendPromptMethod != nullptr) {
            jstring javaResponse = (jstring)env->CallStaticObjectMethod(
                bridgeClass, 
                sendPromptMethod, 
                prompt
            );

            if (javaResponse != nullptr) {
                const char* nativeResponse = env->GetStringUTFChars(javaResponse, nullptr);
                response = nativeResponse;
                env->ReleaseStringUTFChars(javaResponse, nativeResponse);
            }
        }

        env->ReleaseStringUTFChars(prompt, nativePrompt);
    } catch (...) {
        env->ReleaseStringUTFChars(prompt, nativePrompt);
        NEO_LOGE(\"Error in nativeSendGemmaPrompt\");
        return env->NewStringUTF(\"Error processing Gemma prompt\");
    }

    return env->NewStringUTF(response.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeGetTelemetryJSON(
    JNIEnv* env, jclass /*clazz*/) {

    auto& t = NeoJNI::g_Telemetry;
    char buf[512];
    snprintf(buf, sizeof(buf),
        "{\"fps\":%.1f,\"cpuTemp\":%.1f,\"gpuTemp\":%.1f,"
        "\"memoryMB\":%.1f,\"battery\":%.1f,"
        "\"entities\":%d,\"drawCalls\":%d,\"triangles\":%d}",
        t.frameRate, t.cpuTemp, t.gpuTemp,
        t.memoryUsageMB, t.batteryLevel,
        t.activeEntities, t.drawCalls, t.triangleCount);

    return env->NewStringUTF(buf);
}

// =========================================================
// Scene Management Bridge
// =========================================================

JNIEXPORT jint JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeGetActorCount(
    JNIEnv* /*env*/, jclass /*clazz*/) {
    return NeoJNI::g_Telemetry.activeEntities;
}

JNIEXPORT jboolean JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeLoadScene(
    JNIEnv* env, jclass /*clazz*/, jstring scenePath) {

    const char* path = env->GetStringUTFChars(scenePath, nullptr);
    NEO_LOGI("Loading scene: %s", path);
    env->ReleaseStringUTFChars(scenePath, path);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeSaveScene(
    JNIEnv* env, jclass /*clazz*/, jstring scenePath) {

    const char* path = env->GetStringUTFChars(scenePath, nullptr);
    NEO_LOGI("Saving scene: %s", path);
    env->ReleaseStringUTFChars(scenePath, path);
    return JNI_TRUE;
}

} // extern "C"
