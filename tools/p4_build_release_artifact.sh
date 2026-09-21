#!/usr/bin/env bash
set -euo pipefail
umask 077

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

ANDROID_DIR="$ROOT/android"
GRADLEW="$ANDROID_DIR/gradlew"
TASK="${1:-assembleRelease}"

fail() { echo "P4_RELEASE_BUILD_FAIL $*" >&2; exit 2; }

[[ -x "$GRADLEW" && ! -L "$GRADLEW" ]] || fail "missing_gradle_wrapper"
[[ -f "$ANDROID_DIR/gradle/wrapper/gradle-wrapper.properties" && ! -L "$ANDROID_DIR/gradle/wrapper/gradle-wrapper.properties" ]] || fail "incomplete_gradle_wrapper"

case "$TASK" in
  assembleRelease) artifact="$ANDROID_DIR/app/build/outputs/apk/release/app-release.apk" ;;
  bundleRelease) artifact="$ANDROID_DIR/app/build/outputs/bundle/release/app-release.aab" ;;
  clean) fail "clean_not_allowed" ;;
  *) fail "unsupported_task=$TASK" ;;
esac

required=(NEO_ANDROID_KEYSTORE NEO_ANDROID_KEY_ALIAS NEO_ANDROID_STORE_PASSWORD NEO_ANDROID_KEY_PASSWORD)
for name in "${required[@]}"; do
  [[ -n "${!name:-}" ]] || { echo "P4_RELEASE_BUILD_FAIL missing_signing_env=$name" >&2; exit 3; }
done

case "$NEO_ANDROID_KEYSTORE" in *$'\n'*|*$'\r'*|*$'\0'*) echo "P4_RELEASE_BUILD_FAIL unsafe_keystore_path" >&2; exit 3;; esac
[[ -f "$NEO_ANDROID_KEYSTORE" && ! -L "$NEO_ANDROID_KEYSTORE" && -r "$NEO_ANDROID_KEYSTORE" && -s "$NEO_ANDROID_KEYSTORE" ]] || {
  echo "P4_RELEASE_BUILD_FAIL invalid_signing_keystore=$NEO_ANDROID_KEYSTORE" >&2; exit 3;
}

"$GRADLEW" --no-daemon --stacktrace "$TASK"

[[ -f "$artifact" && ! -L "$artifact" && -s "$artifact" && -r "$artifact" && -O "$artifact" ]] || {
  echo "P4_RELEASE_BUILD_FAIL invalid_artifact=$artifact" >&2; exit 4;
}
case "$artifact" in *.apk|*.aab) ;; *) echo "P4_RELEASE_BUILD_FAIL unsupported_artifact=$artifact" >&2; exit 4;; esac

python3 tools/p4_release_artifact_gate.py "$artifact"
printf 'P4_RELEASE_BUILD_OK artifact=%s\n' "$artifact"
