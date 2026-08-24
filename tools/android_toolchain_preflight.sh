#!/usr/bin/env bash
# Toolchain-only preflight. It never invokes Gradle, generates a keystore, or builds APK/AAB.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SDK_ROOT="${ANDROID_HOME:-/home/ubuntu/android-sdk}"
NDK_VERSION="29.0.14206865"
CMAKE_VERSION="3.22.1"
JAVA_HOME_EXPECTED="${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk-amd64}"

fail() { printf 'ANDROID_PREFLIGHT_FAIL reason=%s\n' "$1" >&2; exit 2; }
test -x "$SDK_ROOT/platform-tools/adb" || fail missing_platform_tools
test -x "$SDK_ROOT/ndk/$NDK_VERSION/ndk-build" || fail missing_pinned_ndk
test -x "$SDK_ROOT/cmake/$CMAKE_VERSION/bin/cmake" || fail missing_pinned_cmake
test -d "$SDK_ROOT/platforms/android-36" || fail missing_android_36_platform
test -x "$JAVA_HOME_EXPECTED/bin/java" || fail missing_java_17

CANONICAL_ENGINE="$ROOT/Source/NeoEngine"
GRADLE_MODULE="$ROOT/android/app"
NATIVE_CMAKE="$GRADLE_MODULE/src/main/jni/CMakeLists.txt"
test -d "$CANONICAL_ENGINE" || fail canonical_engine_source_missing
test -f "$GRADLE_MODULE/build.gradle" || fail android_module_missing
test -f "$NATIVE_CMAKE" || fail android_native_cmake_missing

echo "ANDROID_TOOLCHAIN_PREFLIGHT_OK sdk=$SDK_ROOT ndk=$NDK_VERSION cmake=$CMAKE_VERSION java=$JAVA_HOME_EXPECTED"
if grep -Fq '/engine/Source/NeoEngine' "$NATIVE_CMAKE" || ! grep -Fq 'Source/NeoEngine' "$NATIVE_CMAKE"; then
    echo 'ANDROID_PACKAGING_GATE_BLOCKED reason=canonical_native_engine_root_missing'
    echo 'ANDROID_APK_AAB_NOT_BUILT'
    exit 0
fi
if ! grep -Fq "ndkVersion \"$NDK_VERSION\"" "$GRADLE_MODULE/build.gradle"; then
    echo 'ANDROID_PACKAGING_GATE_BLOCKED reason=module_ndk_version_not_pinned'
    echo 'ANDROID_APK_AAB_NOT_BUILT'
    exit 0
fi
if ! grep -Fq 'compileSdk 36' "$GRADLE_MODULE/build.gradle" || ! grep -Fq 'targetSdk 36' "$GRADLE_MODULE/build.gradle"; then
    echo 'ANDROID_PACKAGING_GATE_BLOCKED reason=api_36_module_target_missing'
    echo 'ANDROID_APK_AAB_NOT_BUILT'
    exit 0
fi
echo 'ANDROID_PACKAGING_PREFLIGHT_READY source_alignment=canonical_subset api_target=36 build_not_run=1'
echo 'ANDROID_APK_AAB_NOT_BUILT'
