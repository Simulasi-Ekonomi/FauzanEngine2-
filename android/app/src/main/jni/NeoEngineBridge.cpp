// =========================================================
// NeoEngine JNI Bridge - with Massive World Streaming
// =========================================================

#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <thread>
#include <atomic>
#include <chrono>

#define NEO_JNI_TAG "NeoEngine-JNI"
#define NEO_LOGI(...) __android_log_print(ANDROID_LOG_INFO,  NEO_JNI_TAG, __VA_ARGS__)
#define NEO_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, NEO_JNI_TAG, __VA_ARGS__)
#define NEO_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, NEO_JNI_TAG, __VA_ARGS__)

// Forward declaration
namespace NeoEngine {
    class ProceduralWorldGenerator;
    struct WorldConfig;
}

namespace NeoJNI {

static JavaVM*       g_JavaVM      = nullptr;
static jobject       g_Activity    = nullptr;
static AAssetManager* g_AssetMgr   = nullptr;
static std::mutex    g_Mutex;
static bool          g_Initialized = false;
static bool          g_Running     = false;
static float         g_DeltaTime   = 0.0f;
static int           g_FrameCount  = 0;
static float         g_FPS         = 0.0f;

// World streaming state
static std::unique_ptr<NeoEngine::ProceduralWorldGenerator> g_WorldGenerator;
static std::thread g_StreamingThread;
static std::atomic<bool> g_StreamingActive{false};
static float g_CameraX = 0.0f, g_CameraZ = 0.0f;
static std::mutex g_StreamMutex;
static std::vector<NeoEngine::PlacedObject> g_PendingObjects;

struct Vec3 { float x = 0, y = 0, z = 0; };

struct Actor {
    int         id;
    std::string name;
    std::string type;
    Vec3        position;
    Vec3        rotation;
    Vec3        scale{1,1,1};
    bool        visible = true;
    std::string color   = "#ffffff";
    float       roughness = 0.5f;
    float       metalness = 0.0f;
};

static int                              g_NextActorId = 1;
static std::unordered_map<int, Actor>   g_Actors;
static std::unordered_map<std::string, int> g_ActorsByName;

struct Telemetry {
    float fps       = 60.0f;
    float cpuTemp   = 0.0f;
    float gpuTemp   = 0.0f;
    float memoryMB  = 0.0f;
    float battery   = 100.0f;
    int   entities  = 0;
    int   drawCalls = 0;
    int   triangles = 0;
};
static Telemetry g_Telemetry;

JNIEnv* GetJNIEnv() {
    JNIEnv* env = nullptr;
    if (!g_JavaVM) return nullptr;
    int st = g_JavaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (st == JNI_EDETACHED)
        g_JavaVM->AttachCurrentThread(&env, nullptr);
    return env;
}

static std::string jsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

static std::string actorToJSON(const Actor& a) {
    char buf[512];
    snprintf(buf, sizeof(buf),
        "{\"id\":%d,\"name\":\"%s\",\"type\":\"%s\","
        "\"transform\":{\"position\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f},"
        "\"rotation\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f},"
        "\"scale\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f}},"
        "\"visible\":%s,\"color\":\"%s\",\"roughness\":%.2f,\"metalness\":%.2f}",
        a.id, jsonEscape(a.name).c_str(), a.type.c_str(),
        a.position.x, a.position.y, a.position.z,
        a.rotation.x, a.rotation.y, a.rotation.z,
        a.scale.x,    a.scale.y,    a.scale.z,
        a.visible ? "true" : "false",
        a.color.c_str(), a.roughness, a.metalness
    );
    return buf;
}

} // namespace NeoJNI

// Include ProceduralWorldGenerator setelah namespace
#include "World/ProceduralWorldGenerator.h"

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    NeoJNI::g_JavaVM = vm;
    NEO_LOGI("JNI_OnLoad: NeoEngine native loaded with World Streaming");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM*, void*) {
    NeoJNI::g_StreamingActive = false;
    if (NeoJNI::g_StreamingThread.joinable()) {
        NeoJNI::g_StreamingThread.join();
    }
    NeoJNI::g_WorldGenerator.reset();
    NEO_LOGI("JNI_OnUnload");
    NeoJNI::g_JavaVM = nullptr;
}

// =========================================================
// Massive World Streaming
// =========================================================

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_startWorldStreaming(
    JNIEnv* env, jclass, jint seed, jfloat sizeKm)
{
    std::lock_guard<std::mutex> lk(NeoJNI::g_Mutex);
    NEO_LOGI("startWorldStreaming: seed=%d, size=%.1f km", seed, sizeKm);

    // Stop existing streaming
    if (NeoJNI::g_StreamingActive) {
        NeoJNI::g_StreamingActive = false;
        if (NeoJNI::g_StreamingThread.joinable()) {
            NeoJNI::g_StreamingThread.join();
        }
    }

    // Clear existing actors
    NeoJNI::g_Actors.clear();
    NeoJNI::g_ActorsByName.clear();
    NeoJNI::g_NextActorId = 1;

    // Initialize world generator
    NeoEngine::WorldConfig cfg;
    cfg.worldSizeKm = sizeKm;
    cfg.seed = seed;
    cfg.chunkSizeM = 256.0f;
    cfg.treeDensity = 0.03f;
    cfg.rockDensity = 0.01f;
    cfg.heightScale = 200.0f;

    NeoJNI::g_WorldGenerator = std::make_unique<NeoEngine::ProceduralWorldGenerator>(cfg);
    NeoJNI::g_WorldGenerator->PrecomputeWorld();

    // Start streaming thread
    NeoJNI::g_StreamingActive = true;
    NeoJNI::g_StreamingThread = std::thread([]() {
        NEO_LOGI("World streaming thread started");
        
        while (NeoJNI::g_StreamingActive) {
            // Get camera position
            float camX, camZ;
            {
                std::lock_guard<std::mutex> lk(NeoJNI::g_StreamMutex);
                camX = NeoJNI::g_CameraX;
                camZ = NeoJNI::g_CameraZ;
            }

            // Calculate current chunk
            int chunkSize = static_cast<int>(NeoJNI::g_WorldGenerator->GetConfig().chunkSizeM);
            int camChunkX = static_cast<int>(std::floor(camX / chunkSize));
            int camChunkZ = static_cast<int>(std::floor(camZ / chunkSize));

            // Load chunks in radius 3 (3 chunks = 768m radius)
            for (int dx = -3; dx <= 3; ++dx) {
                for (int dz = -3; dz <= 3; ++dz) {
                    if (!NeoJNI::g_StreamingActive) break;
                    int cx = camChunkX + dx;
                    int cz = camChunkZ + dz;
                    
                    // Check if chunk already loaded (simple check by actor count in area)
                    bool loaded = false;
                    {
                        std::lock_guard<std::mutex> lk(NeoJNI::g_Mutex);
                        // TODO: implement proper chunk tracking
                    }
                    if (loaded) continue;

                    // Generate chunk
                    NeoEngine::WorldChunk chunk = NeoJNI::g_WorldGenerator->GenerateChunk(cx, cz);
                    
                    // Convert to actors and add to scene
                    std::lock_guard<std::mutex> lk(NeoJNI::g_Mutex);
                    for (const auto& obj : chunk.objects) {
                        NeoJNI::Actor a;
                        a.id = NeoJNI::g_NextActorId++;
                        a.name = obj.type + "_" + std::to_string(a.id);
                        a.type = obj.type;
                        a.position = {obj.position.x, obj.position.y, obj.position.z};
                        a.rotation = {obj.rotation.x, obj.rotation.y, obj.rotation.z};
                        a.scale = {obj.scale.x, obj.scale.y, obj.scale.z};
                        
                        NeoJNI::g_Actors[a.id] = a;
                        NeoJNI::g_ActorsByName[a.name] = a.id;
                    }
                    
                    NEO_LOGD("Loaded chunk (%d, %d) with %zu objects", cx, cz, chunk.objects.size());
                }
            }

            // Sleep to avoid overwhelming the device
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        NEO_LOGI("World streaming thread stopped");
    });

    NEO_LOGI("World streaming started");
}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_updateCameraPosition(
    JNIEnv*, jclass, jfloat x, jfloat y, jfloat z)
{
    std::lock_guard<std::mutex> lk(NeoJNI::g_StreamMutex);
    NeoJNI::g_CameraX = x;
    NeoJNI::g_CameraZ = z;
}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_stopWorldStreaming(
    JNIEnv*, jclass)
{
    NEO_LOGI("stopWorldStreaming");
    NeoJNI::g_StreamingActive = false;
    if (NeoJNI::g_StreamingThread.joinable()) {
        NeoJNI::g_StreamingThread.join();
    }
}

// =========================================================
// Engine Core (existing)
// =========================================================

JNIEXPORT jboolean JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeInit(
    JNIEnv* env, jclass,
    jobject activity, jobject assetManager,
    jint w, jint h)
{
    std::lock_guard<std::mutex> lk(NeoJNI::g_Mutex);
    NEO_LOGI("nativeInit %dx%d", w, h);

    NeoJNI::g_Activity  = env->NewGlobalRef(activity);
    NeoJNI::g_AssetMgr  = AAssetManager_fromJava(env, assetManager);

    if (!NeoJNI::g_AssetMgr) {
        NEO_LOGE("nativeInit: no AssetManager");
        env->DeleteGlobalRef(NeoJNI::g_Activity);
        return JNI_FALSE;
    }

    NeoJNI::g_Running     = true;
    NeoJNI::g_Initialized = true;
    NeoJNI::g_FrameCount  = 0;
    NEO_LOGI("nativeInit: OK");
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeShutdown(JNIEnv* env, jclass) {
    std::lock_guard<std::mutex> lk(NeoJNI::g_Mutex);
    NEO_LOGI("nativeShutdown");
    NeoJNI::g_Running     = false;
    NeoJNI::g_Initialized = false;
    if (NeoJNI::g_Activity) {
        env->DeleteGlobalRef(NeoJNI::g_Activity);
        NeoJNI::g_Activity = nullptr;
    }
}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeTick(JNIEnv*, jclass, jfloat dt) {
    if (!NeoJNI::g_Running) return;
    NeoJNI::g_DeltaTime = dt;
    NeoJNI::g_FrameCount++;

    static float fpsTimer  = 0.0f;
    static int   fpsFrames = 0;
    fpsTimer += dt; fpsFrames++;
    if (fpsTimer >= 1.0f) {
        NeoJNI::g_FPS = static_cast<float>(fpsFrames) / fpsTimer;
        NeoJNI::g_Telemetry.fps = NeoJNI::g_FPS;
        NeoJNI::g_Telemetry.entities = static_cast<int>(NeoJNI::g_Actors.size());
        fpsTimer = 0.0f; fpsFrames = 0;
    }
}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeRender(JNIEnv*, jclass) {}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeTouchEvent(
    JNIEnv*, jclass, jint action, jfloat x, jfloat y, jint ptr)
{
    NEO_LOGD("Touch a=%d (%.1f,%.1f) ptr=%d", action, x, y, ptr);
}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeKeyEvent(JNIEnv*, jclass, jint key, jint action) {
    NEO_LOGD("Key key=%d action=%d", key, action);
}

// =========================================================
// Scene Management – called by NeoEngineToolSet (LiteRT)
// =========================================================

JNIEXPORT jint JNICALL
Java_com_neoengine_core_NeoEngineBridgeNative_nativeAddActor(
    JNIEnv* env, jobject,
    jstring jtype, jstring jname,
    jfloat x, jfloat y, jfloat z)
{
    const char* type = env->GetStringUTFChars(jtype, nullptr);
    const char* name = env->GetStringUTFChars(jname, nullptr);

    std::lock_guard<std::mutex> lk(NeoJNI::g_Mutex);
    int id = NeoJNI::g_NextActorId++;

    NeoJNI::Actor a;
    a.id       = id;
    a.name     = name;
    a.type     = type;
    a.position = {x, y, z};

    NeoJNI::g_Actors[id]        = a;
    NeoJNI::g_ActorsByName[name] = id;

    NEO_LOGI("addActor: id=%d name=%s type=%s pos=(%.1f,%.1f,%.1f)", id, name, type, x, y, z);

    env->ReleaseStringUTFChars(jtype, type);
    env->ReleaseStringUTFChars(jname, name);
    return id;
}

JNIEXPORT jboolean JNICALL
Java_com_neoengine_core_NeoEngineBridgeNative_nativeDeleteActor(
    JNIEnv* env, jobject, jstring jname)
{
    const char* name = env->GetStringUTFChars(jname, nullptr);
    std::lock_guard<std::mutex> lk(NeoJNI::g_Mutex);

    auto it = NeoJNI::g_ActorsByName.find(name);
    bool found = (it != NeoJNI::g_ActorsByName.end());
    if (found) {
        NeoJNI::g_Actors.erase(it->second);
        NeoJNI::g_ActorsByName.erase(it);
        NEO_LOGI("deleteActor: %s", name);
    }

    env->ReleaseStringUTFChars(jname, name);
    return found ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridgeNative_nativeSetTransform(
    JNIEnv* env, jobject, jstring jname,
    jfloat px, jfloat py, jfloat pz,
    jfloat rx, jfloat ry, jfloat rz,
    jfloat sx, jfloat sy, jfloat sz)
{
    const char* name = env->GetStringUTFChars(jname, nullptr);
    std::lock_guard<std::mutex> lk(NeoJNI::g_Mutex);

    auto it = NeoJNI::g_ActorsByName.find(name);
    if (it != NeoJNI::g_ActorsByName.end()) {
        auto& a   = NeoJNI::g_Actors[it->second];
        a.position = {px, py, pz};
        a.rotation = {rx, ry, rz};
        a.scale    = {sx, sy, sz};
        NEO_LOGD("setTransform: %s pos=(%.1f,%.1f,%.1f)", name, px, py, pz);
    }

    env->ReleaseStringUTFChars(jname, name);
}

JNIEXPORT jstring JNICALL
Java_com_neoengine_core_NeoEngineBridgeNative_nativeGetSceneJSON(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lk(NeoJNI::g_Mutex);

    std::ostringstream ss;
    ss << "{\"actors\":[";
    bool first = true;
    for (auto& [id, actor] : NeoJNI::g_Actors) {
        if (!first) ss << ",";
        ss << NeoJNI::actorToJSON(actor);
        first = false;
    }
    ss << "],\"actorCount\":" << NeoJNI::g_Actors.size() << "}";

    return env->NewStringUTF(ss.str().c_str());
}

JNIEXPORT jstring JNICALL
Java_com_neoengine_core_NeoEngineBridgeNative_nativeGetTelemetryJSON(JNIEnv* env, jobject) {
    auto& t = NeoJNI::g_Telemetry;
    char buf[256];
    snprintf(buf, sizeof(buf),
        "{\"fps\":%.1f,\"cpuTemp\":%.1f,\"gpuTemp\":%.1f,"
        "\"memoryMB\":%.1f,\"battery\":%.1f,"
        "\"entities\":%d,\"drawCalls\":%d,\"triangles\":%d}",
        t.fps, t.cpuTemp, t.gpuTemp,
        t.memoryMB, t.battery,
        t.entities, t.drawCalls, t.triangles);
    return env->NewStringUTF(buf);
}

// =========================================================
// Telemetry (existing API)
// =========================================================

JNIEXPORT jfloat JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeGetFPS(JNIEnv*, jclass) {
    return NeoJNI::g_FPS;
}

JNIEXPORT jfloat JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeGetCPUTemp(JNIEnv*, jclass) {
    return NeoJNI::g_Telemetry.cpuTemp;
}

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeSetThrottleLevel(JNIEnv*, jclass, jint level) {
    NEO_LOGI("Throttle level=%d (set by Aries)", level);
}

JNIEXPORT jstring JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeGetTelemetryJSON(JNIEnv* env, jclass) {
    auto& t = NeoJNI::g_Telemetry;
    char buf[256];
    snprintf(buf, sizeof(buf),
        "{\"fps\":%.1f,\"cpuTemp\":%.1f,\"gpuTemp\":%.1f,"
        "\"memoryMB\":%.1f,\"battery\":%.1f,"
        "\"entities\":%d,\"drawCalls\":%d,\"triangles\":%d}",
        t.fps, t.cpuTemp, t.gpuTemp,
        t.memoryMB, t.battery,
        t.entities, t.drawCalls, t.triangles);
    return env->NewStringUTF(buf);
}

JNIEXPORT jint JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeGetActorCount(JNIEnv*, jclass) {
    return static_cast<jint>(NeoJNI::g_Actors.size());
}

JNIEXPORT jboolean JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeLoadScene(JNIEnv* env, jclass, jstring path) {
    const char* p = env->GetStringUTFChars(path, nullptr);
    NEO_LOGI("LoadScene: %s", p);
    env->ReleaseStringUTFChars(path, p);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeSaveScene(JNIEnv* env, jclass, jstring path) {
    const char* p = env->GetStringUTFChars(path, nullptr);
    NEO_LOGI("SaveScene: %s", p);
    env->ReleaseStringUTFChars(path, p);
    return JNI_TRUE;
}

// =========================================================
// LiteRT callback
// =========================================================

JNIEXPORT void JNICALL
Java_com_neoengine_core_NeoEngineBridge_nativeOnLiteRTInitialized(JNIEnv*, jclass, jboolean ok) {
    NEO_LOGI("LiteRT initialized: %s", ok ? "OK" : "FAILED");
}

} // extern "C"
