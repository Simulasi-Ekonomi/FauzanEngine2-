#!/usr/bin/env bash
set -euo pipefail

# Final release certification entry point. This is intentionally separate from
# CI: it validates the exact source manifest/SBOM and the exact APK/AAB artifact.
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

ARTIFACT="${1:-}"
if [[ -z "$ARTIFACT" ]]; then
  echo "usage: tools/p4_release_certify.sh <release.apk|release.aab>" >&2
  exit 2
fi

bash tools/release_manifest.sh p4-release-manifest.sha256
python3 tools/generate_source_sbom.py
python3 tools/p4_release_gate.py
python3 tools/p4_release_artifact_gate.py "$ARTIFACT"

sha256sum -- "$ARTIFACT"
printf 'P4_RELEASE_CERTIFICATION_OK artifact=%s commit=%s\n'   "$ARTIFACT" "$(git rev-parse HEAD)"
