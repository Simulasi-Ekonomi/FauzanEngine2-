#!/usr/bin/env bash
set -euo pipefail

# Final release certification entry point. This is intentionally separate from
# CI: it validates the exact source manifest/SBOM and the exact APK/AAB artifact.
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

if [[ -n "$(git status --porcelain=v1 --untracked-files=all)" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL dirty_worktree" >&2
  exit 1
fi

ARTIFACT="${1:-}"
REFERENCE_ARTIFACT="${2:-}"
if [[ "$ARTIFACT" == *
  echo "usage: tools/p4_release_certify.sh <release.apk|release.aab> <reference.apk|reference.aab>" >&2
  exit 2
fi
if [[ ! -f "$ARTIFACT" || -L "$ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL artifact_not_regular_file" >&2
  exit 3
fi
if [[ ! -f "$REFERENCE_ARTIFACT" || -L "$REFERENCE_ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_not_regular_file" >&2
  exit 3
fi
if [[ "$ARTIFACT" == "$REFERENCE_ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_must_be_distinct" >&2
  exit 3
fi
if [[ "$(stat -c %d:%i -- "$ARTIFACT")" == "$(stat -c %d:%i -- "$REFERENCE_ARTIFACT")" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_must_be_distinct_inode" >&2
  exit 3
fi
case "${REFERENCE_ARTIFACT,,}" in
  *.apk|*.aab) ;;
  *) echo "P4_RELEASE_CERTIFICATION_FAIL unsupported_reference_artifact_extension" >&2; exit 3 ;;
esac
artifact_ext="${ARTIFACT##*.}"; artifact_ext="${artifact_ext,,}"
reference_ext="${REFERENCE_ARTIFACT##*.}"; reference_ext="${reference_ext,,}"
if [[ "$artifact_ext" != "$reference_ext" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL artifact_extension_mismatch" >&2
  exit 3
fi
case "${ARTIFACT,,}" in
  *.apk|*.aab) ;;
  *) echo "P4_RELEASE_CERTIFICATION_FAIL unsupported_artifact_extension" >&2; exit 3 ;;
esac
if [[ "$ARTIFACT" == *$'\n'* || "$REFERENCE_ARTIFACT" == *$'\n'* ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL invalid_artifact_path" >&2
  exit 3
fi
if [[ ! -s "$ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL empty_artifact" >&2
  exit 3
fi

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
sha256sum -- "$ARTIFACT"
printf 'P4_RELEASE_CERTIFICATION_OK artifact=%s commit=%s\n'   "$ARTIFACT" "$(git rev-parse HEAD)"
\r'* || "$REFERENCE_ARTIFACT" == *
  echo "usage: tools/p4_release_certify.sh <release.apk|release.aab> <reference.apk|reference.aab>" >&2
  exit 2
fi
if [[ ! -f "$ARTIFACT" || -L "$ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL artifact_not_regular_file" >&2
  exit 3
fi
if [[ ! -f "$REFERENCE_ARTIFACT" || -L "$REFERENCE_ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_not_regular_file" >&2
  exit 3
fi
if [[ "$ARTIFACT" == "$REFERENCE_ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_must_be_distinct" >&2
  exit 3
fi
if [[ "$(stat -c %d:%i -- "$ARTIFACT")" == "$(stat -c %d:%i -- "$REFERENCE_ARTIFACT")" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_must_be_distinct_inode" >&2
  exit 3
fi
case "${REFERENCE_ARTIFACT,,}" in
  *.apk|*.aab) ;;
  *) echo "P4_RELEASE_CERTIFICATION_FAIL unsupported_reference_artifact_extension" >&2; exit 3 ;;
esac
artifact_ext="${ARTIFACT##*.}"; artifact_ext="${artifact_ext,,}"
reference_ext="${REFERENCE_ARTIFACT##*.}"; reference_ext="${reference_ext,,}"
if [[ "$artifact_ext" != "$reference_ext" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL artifact_extension_mismatch" >&2
  exit 3
fi
case "${ARTIFACT,,}" in
  *.apk|*.aab) ;;
  *) echo "P4_RELEASE_CERTIFICATION_FAIL unsupported_artifact_extension" >&2; exit 3 ;;
esac
if [[ "$ARTIFACT" == *$'\n'* || "$REFERENCE_ARTIFACT" == *$'\n'* ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL invalid_artifact_path" >&2
  exit 3
fi
if [[ ! -s "$ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL empty_artifact" >&2
  exit 3
fi

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
sha256sum -- "$ARTIFACT"
printf 'P4_RELEASE_CERTIFICATION_OK artifact=%s commit=%s\n'   "$ARTIFACT" "$(git rev-parse HEAD)"
\r'* || "$ARTIFACT" == *
  echo "usage: tools/p4_release_certify.sh <release.apk|release.aab> <reference.apk|reference.aab>" >&2
  exit 2
fi
if [[ ! -f "$ARTIFACT" || -L "$ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL artifact_not_regular_file" >&2
  exit 3
fi
if [[ ! -f "$REFERENCE_ARTIFACT" || -L "$REFERENCE_ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_not_regular_file" >&2
  exit 3
fi
if [[ "$ARTIFACT" == "$REFERENCE_ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_must_be_distinct" >&2
  exit 3
fi
if [[ "$(stat -c %d:%i -- "$ARTIFACT")" == "$(stat -c %d:%i -- "$REFERENCE_ARTIFACT")" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_must_be_distinct_inode" >&2
  exit 3
fi
case "${REFERENCE_ARTIFACT,,}" in
  *.apk|*.aab) ;;
  *) echo "P4_RELEASE_CERTIFICATION_FAIL unsupported_reference_artifact_extension" >&2; exit 3 ;;
esac
artifact_ext="${ARTIFACT##*.}"; artifact_ext="${artifact_ext,,}"
reference_ext="${REFERENCE_ARTIFACT##*.}"; reference_ext="${reference_ext,,}"
if [[ "$artifact_ext" != "$reference_ext" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL artifact_extension_mismatch" >&2
  exit 3
fi
case "${ARTIFACT,,}" in
  *.apk|*.aab) ;;
  *) echo "P4_RELEASE_CERTIFICATION_FAIL unsupported_artifact_extension" >&2; exit 3 ;;
esac
if [[ "$ARTIFACT" == *$'\n'* || "$REFERENCE_ARTIFACT" == *$'\n'* ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL invalid_artifact_path" >&2
  exit 3
fi
if [[ ! -s "$ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL empty_artifact" >&2
  exit 3
fi

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
sha256sum -- "$ARTIFACT"
printf 'P4_RELEASE_CERTIFICATION_OK artifact=%s commit=%s\n'   "$ARTIFACT" "$(git rev-parse HEAD)"
\0'* || "$REFERENCE_ARTIFACT" == *
  echo "usage: tools/p4_release_certify.sh <release.apk|release.aab> <reference.apk|reference.aab>" >&2
  exit 2
fi
if [[ ! -f "$ARTIFACT" || -L "$ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL artifact_not_regular_file" >&2
  exit 3
fi
if [[ ! -f "$REFERENCE_ARTIFACT" || -L "$REFERENCE_ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_not_regular_file" >&2
  exit 3
fi
if [[ "$ARTIFACT" == "$REFERENCE_ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_must_be_distinct" >&2
  exit 3
fi
if [[ "$(stat -c %d:%i -- "$ARTIFACT")" == "$(stat -c %d:%i -- "$REFERENCE_ARTIFACT")" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_must_be_distinct_inode" >&2
  exit 3
fi
case "${REFERENCE_ARTIFACT,,}" in
  *.apk|*.aab) ;;
  *) echo "P4_RELEASE_CERTIFICATION_FAIL unsupported_reference_artifact_extension" >&2; exit 3 ;;
esac
artifact_ext="${ARTIFACT##*.}"; artifact_ext="${artifact_ext,,}"
reference_ext="${REFERENCE_ARTIFACT##*.}"; reference_ext="${reference_ext,,}"
if [[ "$artifact_ext" != "$reference_ext" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL artifact_extension_mismatch" >&2
  exit 3
fi
case "${ARTIFACT,,}" in
  *.apk|*.aab) ;;
  *) echo "P4_RELEASE_CERTIFICATION_FAIL unsupported_artifact_extension" >&2; exit 3 ;;
esac
if [[ "$ARTIFACT" == *$'\n'* || "$REFERENCE_ARTIFACT" == *$'\n'* ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL invalid_artifact_path" >&2
  exit 3
fi
if [[ ! -s "$ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL empty_artifact" >&2
  exit 3
fi

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
sha256sum -- "$ARTIFACT"
printf 'P4_RELEASE_CERTIFICATION_OK artifact=%s commit=%s\n'   "$ARTIFACT" "$(git rev-parse HEAD)"
\0'* ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL invalid_artifact_path" >&2
  exit 3
fi
if [[ -z "$ARTIFACT" || -z "$REFERENCE_ARTIFACT" ]]; then
  echo "usage: tools/p4_release_certify.sh <release.apk|release.aab> <reference.apk|reference.aab>" >&2
  exit 2
fi
if [[ ! -f "$ARTIFACT" || -L "$ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL artifact_not_regular_file" >&2
  exit 3
fi
if [[ ! -f "$REFERENCE_ARTIFACT" || -L "$REFERENCE_ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_not_regular_file" >&2
  exit 3
fi
if [[ "$ARTIFACT" == "$REFERENCE_ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_must_be_distinct" >&2
  exit 3
fi
if [[ "$(stat -c %d:%i -- "$ARTIFACT")" == "$(stat -c %d:%i -- "$REFERENCE_ARTIFACT")" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL reference_artifact_must_be_distinct_inode" >&2
  exit 3
fi
case "${REFERENCE_ARTIFACT,,}" in
  *.apk|*.aab) ;;
  *) echo "P4_RELEASE_CERTIFICATION_FAIL unsupported_reference_artifact_extension" >&2; exit 3 ;;
esac
artifact_ext="${ARTIFACT##*.}"; artifact_ext="${artifact_ext,,}"
reference_ext="${REFERENCE_ARTIFACT##*.}"; reference_ext="${reference_ext,,}"
if [[ "$artifact_ext" != "$reference_ext" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL artifact_extension_mismatch" >&2
  exit 3
fi
case "${ARTIFACT,,}" in
  *.apk|*.aab) ;;
  *) echo "P4_RELEASE_CERTIFICATION_FAIL unsupported_artifact_extension" >&2; exit 3 ;;
esac
if [[ "$ARTIFACT" == *$'\n'* || "$REFERENCE_ARTIFACT" == *$'\n'* ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL invalid_artifact_path" >&2
  exit 3
fi
if [[ ! -s "$ARTIFACT" ]]; then
  echo "P4_RELEASE_CERTIFICATION_FAIL empty_artifact" >&2
  exit 3
fi

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
sha256sum -- "$ARTIFACT"
printf 'P4_RELEASE_CERTIFICATION_OK artifact=%s commit=%s\n'   "$ARTIFACT" "$(git rev-parse HEAD)"
