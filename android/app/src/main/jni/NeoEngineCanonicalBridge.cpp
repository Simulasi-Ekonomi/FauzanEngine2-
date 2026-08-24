#include <jni.h>

#include "Runtime/AndroidLifecycleGate.h"

namespace { NeoEngine::AndroidLifecycleGate g_lifecycle; }

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
        case 0: accepted = g_lifecycle.Initialize(); break;
        case 1: accepted = g_lifecycle.Resume(); break;
        case 2: accepted = g_lifecycle.Pause(); break;
        case 3: accepted = g_lifecycle.Tick(deltaSeconds); break;
        case 4: accepted = g_lifecycle.Shutdown(); break;
        default: accepted = false; break;
    }
    return accepted ? JNI_TRUE : JNI_FALSE;
}
