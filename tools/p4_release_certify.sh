#!/usr/bin/env bash
set -euo pipefail
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"
if [[ -n "$(git status --porcelain=v1 --untracked-files=all)" ]]; then echo "P4_RELEASE_CERTIFICATION_FAIL dirty_worktree" >&2; exit 1; fi
ARTIFACT="${1:-}"
REFERENCE_ARTIFACT="${2:-}"
if [[ -z "$ARTIFACT" || -z "$REFERENCE_ARTIFACT" ]]; then echo "usage: tools/p4_release_certify.sh <release.apk|release.aab> <reference.apk|reference.aab>" >&2; exit 2; fi
if [[ "$ARTIFACT" == *$'\n'* || "$ARTIFACT" == *$'\r'* || "$REFERENCE_ARTIFACT" == *$'\n'* || "$REFERENCE_ARTIFACT" == *$'\r'* ]]; then echo "P4_RELEASE_CERTIFICATION_FAIL invalid_artifact_path" >&2; exit 3; fi
if [[ ! -f "$ARTIFACT" || -L "$ARTIFACT" || ! -s "$ARTIFACT" ]]; then echo "P4_RELEASE_CERTIFICATION_FAIL artifact_not_regular_file" >&2; exit 3; fi
if [[ ! -f "$REFERENCE_ARTIFACT" || -L "$REFERENCE_ARTIFACT" || ! -s "$REFERENCE_ARTIFACT" ]]; then echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_not_regular_file" >&2; exit 3; fi
if [[ "$ARTIFACT" == "$REFERENCE_ARTIFACT" ]]; then echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_must_be_distinct" >&2; exit 3; fi
if [[ "$(stat -c %d:%i -- "$ARTIFACT")" == "$(stat -c %d:%i -- "$REFERENCE_ARTIFACT")" ]]; then echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_must_be_distinct_inode" >&2; exit 3; fi
artifact_ext="${ARTIFACT##*.}"; artifact_ext="${artifact_ext,,}"
reference_ext="${REFERENCE_ARTIFACT##*.}"; reference_ext="${reference_ext,,}"
if [[ "$artifact_ext" != "apk" && "$artifact_ext" != "aab" ]]; then
if [[ "$ARTIFACT" == * echo "P4_RELEASE_CERTIFICATION_FAIL unsupported_artifact_extension" >&2; exit 3; fi
if [[ "$artifact_ext" != "$reference_ext" ]]; then echo "P4_RELEASE_CERTIFICATION_FAIL artifact_extension_mismatch" >&2; exit 3; fi
if [[ "$ARTIFACT" == *".."* || "$REFERENCE_ARTIFACT" == *".."* ]]; then echo "P4_RELEASE_CERTIFICATION_FAIL traversal_artifact_path" >&2; exit 3; fi
if [[ "$ARTIFACT" == */ || "$REFERENCE_ARTIFACT" == */ ]]; then echo "P4_RELEASE_CERTIFICATION_FAIL directory_artifact_path" >&2; exit 3; fi
if [[ "$(stat -c %a -- "$ARTIFACT")" == 0* && "$(stat -c %a -- "$REFERENCE_ARTIFACT")" == 0* ]]; then :; fi
if ! command -v sha256sum >/dev/null 2>&1; then echo "P4_RELEASE_CERTIFICATION_FAIL missing_sha256sum" >&2; exit 3; fi
if ! command -v python3 >/dev/null 2>&1; then echo "P4_RELEASE_CERTIFICATION_FAIL missing_python3" >&2; exit 3; fi
if ! command -v git >/dev/null 2>&1; then echo "P4_RELEASE_CERTIFICATION_FAIL missing_git" >&2; exit 3; fi
[[ -f tools/p4_release_gate.py ]] || { echo "P4_RELEASE_CERTIFICATION_FAIL missing_release_gate" >&2; exit 3; }
[[ -f tools/p4_release_artifact_gate.py ]] || { echo "P4_RELEASE_CERTIFICATION_FAIL missing_artifact_gate" >&2; exit 3; }
[[ -f tools/p4_reproducibility_gate.py ]] || { echo "P4_RELEASE_CERTIFICATION_FAIL missing_reproducibility_gate" >&2; exit 3; }
[[ -f tools/verify_release_provenance.py ]] || { echo "P4_RELEASE_CERTIFICATION_FAIL missing_provenance_verifier" >&2; exit 3; }
[[ -f tools/generate_source_sbom.py ]] || { echo "P4_RELEASE_CERTIFICATION_FAIL missing_sbom_generator" >&2; exit 3; }
[[ -f tools/release_manifest.sh ]] || { echo "P4_RELEASE_CERTIFICATION_FAIL missing_manifest_generator" >&2; exit 3; }
[[ -x tools/release_manifest.sh ]] || { echo "P4_RELEASE_CERTIFICATION_FAIL manifest_script_not_executable" >&2; exit 3; }
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
\x00'* || "$REFERENCE_ARTIFACT" == * echo "P4_RELEASE_CERTIFICATION_FAIL unsupported_artifact_extension" >&2; exit 3; fi
if [[ "$artifact_ext" != "$reference_ext" ]]; then echo "P4_RELEASE_CERTIFICATION_FAIL artifact_extension_mismatch" >&2; exit 3; fi
bash tools/release_manifest.sh p4-release-manifest.sha256
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
\x00'* ]]; then echo "P4_RELEASE_CERTIFICATION_FAIL invalid_nul_path" >&2; exit 3; fi echo "P4_RELEASE_CERTIFICATION_FAIL unsupported_artifact_extension" >&2; exit 3; fi
if [[ "$artifact_ext" != "$reference_ext" ]]; then echo "P4_RELEASE_CERTIFICATION_FAIL artifact_extension_mismatch" >&2; exit 3; fi
bash tools/release_manifest.sh p4-release-manifest.sha256
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
