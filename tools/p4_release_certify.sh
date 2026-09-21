#!/usr/bin/env bash
set -euo pipefail

# Final release certification entry point. This is intentionally separate from
# CI: it validates the exact source manifest/SBOM and the exact APK/AAB artifact.
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

if ! git diff --quiet --ignore-submodules -- || ! git diff --cached --quiet --ignore-submodules --; then
  echo "P4_RELEASE_CERTIFICATION_FAIL dirty_worktree" >&2
  exit 1
fi

ARTIFACT="${1:-}"
if [[ -z "$ARTIFACT" ]]; then
  echo "usage: tools/p4_release_certify.sh <release.apk|release.aab>" >&2
  exit 2
fi
if [[ ! -f "$ARTIFACT" || -L "$ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL artifact_not_regular_file" >&2
  exit 3
fi
case "$ARTIFACT" in
  *.apk|*.aab) ;;
  *) echo "P4_RELEASE_CERTIFICATION_FAIL unsupported_artifact_extension" >&2; exit 3 ;;
esac
if [[ ! -s "$ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL empty_artifact" >&2
  exit 3
fi

bash tools/release_manifest.sh p4-release-manifest.sha256
python3 tools/generate_source_sbom.py
python3 tools/p4_release_gate.py
python3 tools/p4_release_artifact_gate.py "$ARTIFACT"

ARTIFACT_SHA256="$(sha256sum -- "$ARTIFACT" | awk '{print $1}')"
MANIFEST_SHA256="$(sha256sum -- p4-release-manifest.sha256 | awk '{print $1}')"
SBOM_SHA256="$(sha256sum -- p4-source-sbom.json | awk '{print $1}')"
PROVENANCE="p4-release-provenance.txt"
{
  printf 'FAUZANENGINE_RELEASE_PROVENANCE_V1\n'
  printf 'commit=%s\n' "$(git rev-parse HEAD)"
  printf 'tree=%s\n' "$(git rev-parse HEAD^{tree})"
  printf 'artifact=%s\n' "$ARTIFACT"
  printf 'artifact_sha256=%s\n' "$ARTIFACT_SHA256"
  printf 'manifest_sha256=%s\n' "$MANIFEST_SHA256"
  printf 'sbom_sha256=%s\n' "$SBOM_SHA256"
} > "$PROVENANCE"

python3 tools/verify_release_provenance.py "$ARTIFACT"
sha256sum -- "$ARTIFACT"
printf 'P4_RELEASE_CERTIFICATION_OK artifact=%s commit=%s\n'   "$ARTIFACT" "$(git rev-parse HEAD)"
