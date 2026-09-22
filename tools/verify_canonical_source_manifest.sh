#!/usr/bin/env bash
set -euo pipefail
cmake_file="Source/NeoEngine/CMakeLists.txt"
[[ -f "$cmake_file" ]] || { echo "CANONICAL_SOURCE_MANIFEST_FAIL missing_cmake"; exit 1; }
mapfile -t sources < <(sed -n '/set(XPBD_RUNTIME_SOURCES/,/^)/p' "$cmake_file" | sed -n 's/^    \(.*\.cpp\)$/\1/p')
declare -A seen
for src in "${sources[@]}"; do
  [[ -n "${src}" ]] || continue
  if [[ "${seen[$src]+x}" == x ]]; then
    echo "CANONICAL_SOURCE_MANIFEST_FAIL duplicate=$src"
    exit 1
  fi
  seen["$src"]=1
  [[ -f "Source/NeoEngine/$src" ]] || { echo "CANONICAL_SOURCE_MANIFEST_FAIL missing=Source/NeoEngine/$src"; exit 1; }
done
count=${#sources[@]}
(( count > 0 )) || { echo "CANONICAL_SOURCE_MANIFEST_FAIL empty"; exit 1; }
echo "CANONICAL_SOURCE_MANIFEST_OK entries=$count duplicates=0 missing=0"
