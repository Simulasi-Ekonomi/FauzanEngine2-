#!/usr/bin/env bash
set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

ANDROID_DIR="$ROOT/android"
GRADLEW="$ANDROID_DIR/gradlew"
TASK="${1:-assembleRelease}"

if [[ ! -x "$GRADLEW" ]]; then
  echo "P4_RELEASE_BUILD_FAIL missing_gradle_wrapper" >&2
  exit 2
fi

case "$TASK" in
  assembleRelease|bundleRelease) ;;
  *) echo "P4_RELEASE_BUILD_FAIL unsupported_task=$TASK" >&2; exit 2 ;;
esac

required=(NEO_ANDROID_KEYSTORE NEO_ANDROID_KEY_ALIAS NEO_ANDROID_STORE_PASSWORD NEO_ANDROID_KEY_PASSWORD)
for name in "${required[@]}"; do
  if [[ -z "${!name:-}" ]]; then
    echo "P4_RELEASE_BUILD_FAIL missing_signing_env=$name" >&2
    exit 3
  fi
done

"$GRADLEW" --no-daemon --stacktrace "$TASK"

case "$TASK" in
  assembleRelease)
    artifact="$ANDROID_DIR/app/build/outputs/apk/release/app-release.apk"
    ;;
  bundleRelease)
    artifact="$ANDROID_DIR/app/build/outputs/bundle/release/app-release.aab"
    ;;
esac

if [[ ! -f "$artifact" || -L "$artifact" || ! -s "$artifact" ]]; then
  echo "P4_RELEASE_BUILD_FAIL missing_artifact=$artifact" >&2
  exit 4
fi

python3 tools/p4_release_artifact_gate.py "$artifact"
printf 'P4_RELEASE_BUILD_OK artifact=%s\n' "$artifact"
