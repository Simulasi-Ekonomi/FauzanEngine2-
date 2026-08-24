#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="${1:-debug}"
errors=0

fail() { printf 'PRECHECK_FAIL %s\n' "$1" >&2; errors=$((errors + 1)); }
pass() { printf 'PRECHECK_OK %s\n' "$1"; }

SDK_ROOT="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
if [[ -z "$SDK_ROOT" && -f "$ROOT/local.properties" ]]; then SDK_ROOT="$(sed -n 's/^sdk.dir=//p' "$ROOT/local.properties" | head -1)"; fi
if [[ -n "$SDK_ROOT" && -d "$SDK_ROOT" ]]; then pass "android_sdk=$SDK_ROOT"; else fail "android_sdk_missing_or_stale"; fi

if command -v java >/dev/null 2>&1 && java -version 2>&1 | grep -q '"17\.'; then pass "java_17"; else fail "java_17_required"; fi
if [[ -f "$ROOT/gradle/wrapper/gradle-wrapper.jar" ]]; then pass "gradle_wrapper_jar"; else fail "gradle_wrapper_jar_missing"; fi
if [[ -n "$SDK_ROOT" && -d "$SDK_ROOT/cmake/3.22.1" ]]; then pass "cmake_3_22_1"; else fail "android_cmake_3_22_1_missing"; fi
if [[ -n "$SDK_ROOT" ]] && compgen -G "$SDK_ROOT/ndk/*" >/dev/null; then pass "android_ndk"; else fail "android_ndk_missing"; fi
if [[ ! -f "$ROOT/app/src/main/jni/CMakeLists.txt" ]]; then fail "jni_cmake_missing"; else pass "jni_cmake"; fi

if [[ "$MODE" == "release" ]]; then
  for key in NEO_ANDROID_KEYSTORE NEO_ANDROID_KEY_ALIAS NEO_ANDROID_STORE_PASSWORD NEO_ANDROID_KEY_PASSWORD; do
    [[ -n "${!key:-}" ]] && pass "${key}_present" || fail "${key}_missing"
  done
fi

if (( errors > 0 )); then
  printf 'PRECHECK_BLOCKED mode=%s errors=%d\n' "$MODE" "$errors" >&2
  exit 2
fi
printf 'PRECHECK_READY mode=%s\n' "$MODE"
