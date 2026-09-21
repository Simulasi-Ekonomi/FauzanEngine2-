#!/usr/bin/env bash
set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

ANDROID_DIR="$ROOT/android"
GRADLEW="$ANDROID_DIR/gradlew"

[[ -x "$GRADLEW" ]] || { echo "ANDROID_RELEASE_BUILD_FAIL missing_gradle_wrapper" >&2; exit 2; }

"$ROOT/android/scripts/preflight.sh" release
"$ROOT/tools/android_lifecycle_bridge_source_smoke.sh"

for name in NEO_ANDROID_KEYSTORE NEO_ANDROID_KEY_ALIAS NEO_ANDROID_STORE_PASSWORD NEO_ANDROID_KEY_PASSWORD; do
  value="${!name:-}"
  [[ "$value" =~ [^[:space:]] ]] || { echo "ANDROID_RELEASE_BUILD_FAIL missing_signing_env=$name" >&2; exit 3; }
done

keystore="${NEO_ANDROID_KEYSTORE}"
[[ -f "$keystore" && ! -L "$keystore" && -s "$keystore" ]] || {
  echo "ANDROID_RELEASE_BUILD_FAIL invalid_keystore=$keystore" >&2
  exit 3
}

"$GRADLEW" --no-daemon --stacktrace assembleRelease

artifact="$ANDROID_DIR/app/build/outputs/apk/release/app-release.apk"
[[ -f "$artifact" && ! -L "$artifact" && -s "$artifact" ]] || {
  echo "ANDROID_RELEASE_BUILD_FAIL missing_artifact=$artifact" >&2
  exit 4
}

if command -v apksigner >/dev/null 2>&1; then
  apksigner verify --verbose --print-certs "$artifact"
  python3 "$ROOT/tools/p4_release_artifact_gate.py" "$artifact"
else
  echo "ANDROID_RELEASE_BUILD_FAIL missing_tool=apksigner" >&2
  exit 5
fi

printf 'ANDROID_RELEASE_BUILD_OK artifact=%s\n' "$artifact"
