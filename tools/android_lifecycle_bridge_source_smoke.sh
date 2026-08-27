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
grep -q 'NeoEngineCanonicalBridge.pause()' "$activity"
grep -q 'NeoEngineCanonicalBridge.shutdown()' "$activity"
grep -q 'private static native boolean nativeLifecycleEvent' "$bridge"
grep -q 'Java_com_neoengine_core_NeoEngineCanonicalBridge_nativeLifecycleEvent' "$jni_bridge"
grep -q 'LITERT_UNAVAILABLE' "$legacy_bridge"
grep -q 'World streaming is unavailable' "$legacy_bridge"
if grep -q 'startWorldStreaming' "$activity" || grep -q '/sdcard/Gemma4' "$activity" || grep -q 'initLiteRT' "$activity"; then
    echo 'android-lifecycle-bridge: FAIL: Activity retained an inactive streaming or LiteRT execution path' >&2
    exit 1
fi
echo 'android-lifecycle-bridge: PASS: create/resume/pause/destroy route only to canonical JNI lifecycle; LiteRT and world streaming remain fail-closed'
