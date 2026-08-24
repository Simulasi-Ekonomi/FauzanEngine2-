#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output="$(ANDROID_HOME=/home/ubuntu/android-sdk JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64 bash "$ROOT/tools/android_toolchain_preflight.sh")"
printf '%s\n' "$output"
grep -Fq 'ANDROID_TOOLCHAIN_PREFLIGHT_OK' <<<"$output"
grep -Fq 'ANDROID_PACKAGING_GATE_BLOCKED reason=legacy_native_engine_root_path' <<<"$output"
grep -Fq 'ANDROID_APK_AAB_NOT_BUILT' <<<"$output"
echo 'ANDROID_TOOLCHAIN_PREFLIGHT_SMOKE_OK toolchain=1 packagingBlocked=1'
