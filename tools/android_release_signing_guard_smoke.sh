#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_FILE="$ROOT/android/app/build.gradle"

required=(
  NEO_ANDROID_KEYSTORE
  NEO_ANDROID_KEY_ALIAS
  NEO_ANDROID_STORE_PASSWORD
  NEO_ANDROID_KEY_PASSWORD
)
for key in "${required[@]}"; do
  grep -Fq "'$key'" "$BUILD_FILE"
done
grep -Fq "Release APK/AAB requires NEO_ANDROID_KEYSTORE" "$BUILD_FILE"
grep -Fq "signingValues.any" "$BUILD_FILE"
grep -Fq "signingConfig signingConfigs.release" "$BUILD_FILE"
printf 'ANDROID_RELEASE_SIGNING_GUARD_SMOKE_OK required_keys=4 fail_closed=1 secrets_read=0\n'
