#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
activity="$repo_root/android/app/src/main/java/com/neoengine/core/NeoEngineActivity.java"
bridge="$repo_root/android/app/src/main/java/com/neoengine/core/NeoEngineCanonicalBridge.java"
jni_bridge="$repo_root/android/app/src/main/jni/NeoEngineCanonicalBridge.cpp"
legacy_bridge="$repo_root/android/app/src/main/java/com/neoengine/core/NeoEngineBridge.java"

for path in "$activity" "$bridge" "$jni_bridge" "$legacy_bridge"; do test -f "$path"; done
grep -q 'nativeInitialized = NeoEngineCanonicalBridge.initialize();' "$activity"
grep -q 'NeoEngineCanonicalBridge.resume()' "$activity"
grep -q 'NeoEngineCanonicalBridge.tick(deltaSeconds)' "$activity"
grep -q 'Choreographer.getInstance().postFrameCallback' "$activity"
grep -q 'Choreographer.getInstance().removeFrameCallback' "$activity"
grep -q 'NeoEngineCanonicalBridge.pause()' "$activity"
grep -q 'NeoEngineCanonicalBridge.shutdown()' "$activity"
grep -q 'protected void onDestroy()' "$activity"
grep -q 'nativeInitialized = false;' "$activity"
grep -q 'webView = null;' "$activity"
grep -q 'super.onDestroy();' "$activity"
grep -q 'android:usesCleartextTraffic="false"' "$repo_root/android/app/src/main/AndroidManifest.xml"
grep -q 'private static native boolean nativeLifecycleEvent' "$bridge"
grep -q 'Java_com_neoengine_core_NeoEngineCanonicalBridge_nativeLifecycleEvent' "$jni_bridge"
grep -q 'case 0: accepted = g_lifecycle.Initialize();' "$jni_bridge"
grep -q 'case 1: accepted = g_lifecycle.Resume();' "$jni_bridge"
grep -q 'case 2: accepted = g_lifecycle.Pause();' "$jni_bridge"
grep -q 'case 3: accepted = g_lifecycle.Tick(deltaSeconds);' "$jni_bridge"
grep -q 'case 4: accepted = g_lifecycle.Shutdown();' "$jni_bridge"
grep -q 'public static boolean initialize() { return invoke(0, 0.0f); }' "$bridge"
grep -q 'public static boolean resume() { return invoke(1, 0.0f); }' "$bridge"
grep -q 'public static boolean pause() { return invoke(2, 0.0f); }' "$bridge"
grep -q 'public static boolean tick(float deltaSeconds) { return invoke(3, deltaSeconds); }' "$bridge"
grep -q 'public static boolean shutdown() { return invoke(4, 0.0f); }' "$bridge"
grep -q 'LITERT_UNAVAILABLE' "$legacy_bridge"
grep -q 'World streaming is unavailable' "$legacy_bridge"
if grep -q 'startWorldStreaming' "$activity" || grep -q '/sdcard/Gemma4' "$activity" || grep -q 'initLiteRT' "$activity"; then
    echo 'android-lifecycle-bridge: FAIL: Activity retained an inactive streaming or LiteRT execution path' >&2
    exit 1
fi
echo 'android-lifecycle-bridge: PASS: create/resume/pause/destroy route only to canonical JNI lifecycle; LiteRT and world streaming remain fail-closed'