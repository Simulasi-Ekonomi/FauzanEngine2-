#!/usr/bin/env bash
set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

if [[ $# -ne 1 ]]; then
    echo "RELEASE_MANIFEST_FAIL usage" >&2
    exit 2
fi
OUT="$1"

if [[ -z "$OUT" || "$OUT" == -* || "$OUT" == *$'\n'* || "$OUT" == *$'\r'* || "$OUT" == ../* || "$OUT" == */../* ]]; then
    echo "RELEASE_MANIFEST_FAIL unsafe_output" >&2
    exit 2
fi
if [[ -L "$OUT" ]]; then
    echo "RELEASE_MANIFEST_FAIL output_symlink" >&2
    exit 2
fi

TMP="$(mktemp)"
trap 'rm -f "$TMP"' EXIT

git ls-files -z ':!*.keystore' ':!*.jks' ':!*.p12' ':!build/' ':!out/' ':!dist/' ':!.git/' |
while IFS= read -r -d '' path; do
    if [[ -f "$path" && ! -L "$path" ]]; then
        sha256sum -- "$path"
    fi
done | LC_ALL=C sort -k2,2 > "$TMP"

{
    printf 'FAUZANENGINE_RELEASE_MANIFEST_V1\n'
    printf 'commit=%s\n' "$(git rev-parse HEAD)"
    printf 'tree=%s\n' "$(git rev-parse HEAD^{tree})"
    printf 'files=%s\n' "$(wc -l < "$TMP")"
    cat "$TMP"
} > "$OUT"

if [[ ! -f "$OUT" || -L "$OUT" || ! -s "$OUT" ]]; then
    echo "RELEASE_MANIFEST_FAIL output_invalid" >&2
    exit 2
fi

printf 'RELEASE_MANIFEST_OK commit=%s files=%s output=%s\n'     "$(git rev-parse --short HEAD)" "$(wc -l < "$TMP")" "$OUT"
