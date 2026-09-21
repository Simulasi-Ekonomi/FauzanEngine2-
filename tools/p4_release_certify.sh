#!/usr/bin/env bash
set -euo pipefail
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"
fail(){ echo "P4_RELEASE_CERTIFICATION_FAIL $1" >&2; exit 3; }
[[ -z "$(git status --porcelain=v1 --untracked-files=all)" ]] || fail dirty_worktree
ARTIFACT="${1:-}"; REFERENCE_ARTIFACT="${2:-}"
[[ -n "$ARTIFACT" && -n "$REFERENCE_ARTIFACT" ]] || { echo "usage: tools/p4_release_certify.sh <release.apk|release.aab> <reference.apk|reference.aab>" >&2; exit 2; }
for path in "$ARTIFACT" "$REFERENCE_ARTIFACT"; do
  [[ "$path" != *$'\n'* && "$path" != *$'\r'* ]] || fail invalid_artifact_path
  [[ "$path" != *".."* ]] || fail traversal_artifact_path
  [[ "$path" != */ ]] || fail directory_artifact_path
  [[ -f "$path" && ! -L "$path" && -s "$path" ]] || fail artifact_not_regular_file
  [[ "$path" == "$ROOT"/* || "$path" != /* ]] || fail artifact_path_resolution
done
[[ "$ARTIFACT" != "$REFERENCE_ARTIFACT" ]] || fail reference_artifact_must_be_distinct
[[ "$(stat -c %d:%i -- "$ARTIFACT")" != "$(stat -c %d:%i -- "$REFERENCE_ARTIFACT")" ]] || fail reference_artifact_must_be_distinct_inode
artifact_ext="${ARTIFACT##*.}"; artifact_ext="${artifact_ext,,}"
reference_ext="${REFERENCE_ARTIFACT##*.}"; reference_ext="${reference_ext,,}"
[[ "$artifact_ext" == "apk" || "$artifact_ext" == "aab" ]] || fail unsupported_artifact_extension
[[ "$artifact_ext" == "$reference_ext" ]] || fail artifact_extension_mismatch
for tool in git python3 sha256sum bash stat; do command -v "$tool" >/dev/null 2>&1 || fail "missing_tool_$tool"; done
for required in tools/p4_release_gate.py tools/p4_release_artifact_gate.py tools/p4_reproducibility_gate.py tools/verify_release_provenance.py tools/generate_source_sbom.py tools/release_manifest.sh; do
  [[ -f "$required" ]] || fail "missing_required_tool_$required"
done
[[ -x tools/release_manifest.sh ]] || fail manifest_script_not_executable
bash tools/release_manifest.sh p4-release-manifest.sha256
python3 -m py_compile tools/generate_source_sbom.py tools/p4_release_gate.py tools/p4_release_artifact_gate.py tools/p4_reproducibility_gate.py tools/verify_release_provenance.py
python3 tools/generate_source_sbom.py
python3 tools/p4_release_gate.py
python3 tools/p4_release_artifact_gate.py "$ARTIFACT"
python3 tools/p4_release_artifact_gate.py "$REFERENCE_ARTIFACT"
python3 tools/p4_reproducibility_gate.py "$ARTIFACT" "$REFERENCE_ARTIFACT"
ARTIFACT_SHA256="$(sha256sum -- "$ARTIFACT" | awk '{print $1}')"
REFERENCE_ARTIFACT_SHA256="$(sha256sum -- "$REFERENCE_ARTIFACT" | awk '{print $1}')"
MANIFEST_SHA256="$(sha256sum -- p4-release-manifest.sha256 | awk '{print $1}')"
SBOM_SHA256="$(sha256sum -- p4-source-sbom.json | awk '{print $1}')"
PROVENANCE="p4-release-provenance.txt"
{
 printf 'FAUZANENGINE_RELEASE_PROVENANCE_V1\n'
 printf 'commit=%s\n' "$(git rev-parse HEAD)"
 printf 'tree=%s\n' "$(git rev-parse HEAD^{tree})"
 printf 'artifact=%s\n' "$ARTIFACT"
 printf 'artifact_sha256=%s\n' "$ARTIFACT_SHA256"
 printf 'reference_artifact=%s\n' "$REFERENCE_ARTIFACT"
 printf 'reference_artifact_sha256=%s\n' "$REFERENCE_ARTIFACT_SHA256"
 printf 'manifest_sha256=%s\n' "$MANIFEST_SHA256"
 printf 'sbom_sha256=%s\n' "$SBOM_SHA256"
} > "$PROVENANCE"
python3 tools/verify_release_provenance.py "$ARTIFACT"
printf 'P4_RELEASE_CERTIFICATION_OK artifact=%s commit=%s\n' "$ARTIFACT" "$(git rev-parse HEAD)"
