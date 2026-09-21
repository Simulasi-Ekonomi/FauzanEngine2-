#include <jni.h>
#include <cstdint>\n#include <cmath>\n#include <limits>

#include "Runtime/AndroidLifecycleGate.h"
#include "Runtime/SoftwareRenderer.h"

namespace { NeoEngine::AndroidLifecycleGate g_lifecycle; NeoEngine::SoftwareRenderer g_renderer; bool g_rendererReady = false; }

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM*, void*) {
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_neoengine_core_NeoEngineCanonicalBridge_nativeCoreProfile(JNIEnv* env, jclass) {
    return env->NewStringUTF("canonical-runtime-persistence-economy-lifecycle-subset-v1");
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_neoengine_core_NeoEngineCanonicalBridge_nativeLifecycleEvent(JNIEnv*, jclass, jint eventCode, jfloat deltaSeconds) {
    bool accepted = false;
    switch (eventCode) {
        case 0:
            accepted = g_lifecycle.Initialize();
            if (accepted) g_rendererReady = g_renderer.Initialize(256U, 256U);
            accepted = accepted && g_rendererReady;
            break;
        case 1: accepted = g_lifecycle.Resume(); break;
        case 2: accepted = g_lifecycle.Pause(); break;
        case 3: accepted = g_lifecycle.Tick(deltaSeconds); break;
        case 4:
            g_rendererReady = false;
            g_renderer = NeoEngine::SoftwareRenderer{};
            accepted = g_lifecycle.Shutdown();
            break;
        default: accepted = false; break;
    }
    return accepted ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_neoengine_core_NeoEngineCanonicalBridge_nativeRenderFrame(JNIEnv*, jclass) {
    if (!g_lifecycle.IsRunning() || !g_rendererReady) return 0L;
    if (g_renderer.Width() == 0U || g_renderer.Height() == 0U) return 0L;
    if (g_renderer.Width() > 4096U || g_renderer.Height() > 4096U) return 0L;
    if (static_cast<uint64_t>(g_renderer.Width()) * static_cast<uint64_t>(g_renderer.Height()) > 16ULL * 1024ULL * 1024ULL) return 0L;
    if (!g_renderer.Clear(0xFF101820U)) return 0L;
    const NeoEngine::RenderVertex a{32.0F, 32.0F, 0.2F, 0xFFFFAA33U};
    const NeoEngine::RenderVertex b{224.0F, 48.0F, 0.2F, 0xFF33AAFFU};
    const NeoEngine::RenderVertex d{128.0F, 224.0F, 0.2F, 0xFF66DD88U};
    if (!std::isfinite(a.x) || !std::isfinite(a.y) || !std::isfinite(a.z) || !std::isfinite(b.x) || !std::isfinite(b.y) || !std::isfinite(b.z) || !std::isfinite(d.x) || !std::isfinite(d.y) || !std::isfinite(d.z)) return 0L;
    if (!g_renderer.DrawTriangle(a, b, d, false, true)) return 0L;
    const uint64_t hash = g_renderer.FrameHash();
    if (hash == 0U || hash == std::numeric_limits<uint64_t>::max()) return 0L;
    if (g_renderer.Pixels().empty()) return 0L;
    if (g_renderer.Pixels().size() != static_cast<size_t>(g_renderer.Width()) * static_cast<size_t>(g_renderer.Height())) return 0L;
    return static_cast<jlong>(hash);
}
