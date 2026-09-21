#!/usr/bin/env bash
set -euo pipefail
umask 077

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

ANDROID_DIR="$ROOT/android"
GRADLEW="$ANDROID_DIR/gradlew"
TASK="${1:-assembleRelease}"

if [[ ! -x "$GRADLEW" || -L "$GRADLEW" ]]; then
  echo "P4_RELEASE_BUILD_FAIL missing_gradle_wrapper" >&2
  exit 2
fi

case "$TASK" in
  assembleRelease|bundleRelease) ;;
  clean) echo "P4_RELEASE_BUILD_FAIL clean_not_allowed"; exit 2 ;;

  *) echo "P4_RELEASE_BUILD_FAIL unsupported_task=$TASK" >&2; exit 2 ;;
esac

required=(NEO_ANDROID_KEYSTORE NEO_ANDROID_KEY_ALIAS NEO_ANDROID_STORE_PASSWORD NEO_ANDROID_KEY_PASSWORD)
for name in "${required[@]}"; do
  if [[ -z "${!name:-}" ]]; then
    echo "P4_RELEASE_BUILD_FAIL missing_signing_env=$name" >&2
    exit 3
  fi
done

if [[ "$NEO_ANDROID_KEYSTORE" == *
  echo "P4_RELEASE_BUILD_FAIL invalid_signing_keystore=$NEO_ANDROID_KEYSTORE" >&2
  exit 3
fi

if [[ ! -x "$ANDROID_DIR/gradlew" || ! -f "$ANDROID_DIR/gradle/wrapper/gradle-wrapper.properties" ]]; then
  echo "P4_RELEASE_BUILD_FAIL incomplete_gradle_wrapper" >&2
  exit 2
fi

"$GRADLEW" --no-daemon --stacktrace "$TASK"

case "$TASK" in
  assembleRelease)
    artifact="$ANDROID_DIR/app/build/outputs/apk/release/app-release.apk"
    ;;
  bundleRelease)
    artifact="$ANDROID_DIR/app/build/outputs/bundle/release/app-release.aab"
    ;;
esac

if [[ ! -f "$artifact" || -L "$artifact" || ! -s "$artifact" || ! -r "$artifact" || ! -O "$artifact" ]]; then
  echo "P4_RELEASE_BUILD_FAIL missing_artifact=$artifact" >&2
  exit 4
fi

python3 tools/p4_release_artifact_gate.py "$artifact"
printf 'P4_RELEASE_BUILD_OK artifact=%s\n' "$artifact"
\n'* || "$NEO_ANDROID_KEYSTORE" == *
  echo "P4_RELEASE_BUILD_FAIL invalid_signing_keystore=$NEO_ANDROID_KEYSTORE" >&2
  exit 3
fi

if [[ ! -x "$ANDROID_DIR/gradlew" || ! -f "$ANDROID_DIR/gradle/wrapper/gradle-wrapper.properties" ]]; then
  echo "P4_RELEASE_BUILD_FAIL incomplete_gradle_wrapper" >&2
  exit 2
fi

"$GRADLEW" --no-daemon --stacktrace "$TASK"

case "$TASK" in
  assembleRelease)
    artifact="$ANDROID_DIR/app/build/outputs/apk/release/app-release.apk"
    ;;
  bundleRelease)
    artifact="$ANDROID_DIR/app/build/outputs/bundle/release/app-release.aab"
    ;;
esac

if [[ ! -f "$artifact" || -L "$artifact" || ! -s "$artifact" || ! -r "$artifact" || ! -O "$artifact" ]]; then
  echo "P4_RELEASE_BUILD_FAIL missing_artifact=$artifact" >&2
  exit 4
fi

python3 tools/p4_release_artifact_gate.py "$artifact"
printf 'P4_RELEASE_BUILD_OK artifact=%s\n' "$artifact"
\r'* ]]; then echo "P4_RELEASE_BUILD_FAIL unsafe_keystore_path" >&2; exit 3; fi
if [[ ! -f "$NEO_ANDROID_KEYSTORE" || -L "$NEO_ANDROID_KEYSTORE" || ! -r "$NEO_ANDROID_KEYSTORE" || ! -s "$NEO_ANDROID_KEYSTORE" ]]; then
  echo "P4_RELEASE_BUILD_FAIL invalid_signing_keystore=$NEO_ANDROID_KEYSTORE" >&2
  exit 3
fi

if [[ ! -x "$ANDROID_DIR/gradlew" || ! -f "$ANDROID_DIR/gradle/wrapper/gradle-wrapper.properties" ]]; then
  echo "P4_RELEASE_BUILD_FAIL incomplete_gradle_wrapper" >&2
  exit 2
fi

"$GRADLEW" --no-daemon --stacktrace "$TASK"

case "$TASK" in
  assembleRelease)
    artifact="$ANDROID_DIR/app/build/outputs/apk/release/app-release.apk"
    ;;
  bundleRelease)
    artifact="$ANDROID_DIR/app/build/outputs/bundle/release/app-release.aab"
    ;;
esac

if [[ ! -f "$artifact" || -L "$artifact" || ! -s "$artifact" || ! -r "$artifact" || ! -O "$artifact" ]]; then
  echo "P4_RELEASE_BUILD_FAIL missing_artifact=$artifact" >&2
  exit 4
fi

python3 tools/p4_release_artifact_gate.py "$artifact"
printf 'P4_RELEASE_BUILD_OK artifact=%s\n' "$artifact"
