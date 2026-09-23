#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-Build/p2_physics_networking}"
BUILD_TYPE="${P2_BUILD_TYPE:-Release}"
REPORT_DIR="${BUILD_DIR}/sandbox-report"
mkdir -p "$REPORT_DIR"

cmake -S Source/NeoEngine -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_CXX_FLAGS="${P2_CMAKE_CXX_FLAGS:-}" \
  -DCMAKE_EXE_LINKER_FLAGS="${P2_CMAKE_EXE_LINKER_FLAGS:-}"

mapfile -t TARGETS < <(cmake --build "$BUILD_DIR" --target help | awk '/^[[:space:]]+[A-Za-z0-9_.+-]+_smoke([[:space:]]|$)/ {print $1}' | sort -u)
printf "%s\n" "${TARGETS[@]}" >"$REPORT_DIR/targets.txt"
echo "[P2-SANDBOX] discovered ${#TARGETS[@]} CMake smoke targets"

cmake --build "$BUILD_DIR" --target ${TARGETS[*]} -j"${P2_BUILD_JOBS:-2}"

: >"$REPORT_DIR/execution.txt"
pass=0
fail=0
for target in "${TARGETS[@]}"; do
  if SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-dummy}" SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-dummy}" "$BUILD_DIR/$target" >>"$REPORT_DIR/execution.log" 2>&1; then
    printf "%s\tPASS\n" "$target" | tee -a "$REPORT_DIR/execution.txt"
    pass=$((pass+1))
  else
    rc=$?
    printf "%s\tFAIL\texit=%d\n" "$target" "$rc" | tee -a "$REPORT_DIR/execution.txt"
    fail=$((fail+1))
  fi
done

printf "P2 physics/networking complete coverage\ntargets=%d\npass=%d\nfail=%d\nskip=0\n" "${#TARGETS[@]}" "$pass" "$fail" | tee "$REPORT_DIR/summary.txt"

git fetch --no-tags --depth=1 origin main
BASE_SHA="$(git rev-parse origin/main)"
git diff --name-only "$BASE_SHA"...HEAD -- 'Source/NeoEngine' 'Tests' >"$REPORT_DIR/changed_files.txt"
: >"$REPORT_DIR/static_markers.txt"
while IFS= read -r file; do
  [[ -f "$file" ]] || continue
  grep -nEi '^[[:space:]]*(//|/\\*|#)[[:space:]]*(TODO|FIXME|XXX|placeholder|not implemented|IMPLEMENT_ME|stub)\\b' "$file" >>"$REPORT_DIR/static_markers.txt" || true
done <"$REPORT_DIR/changed_files.txt"
if [[ -s "$REPORT_DIR/static_markers.txt" ]]; then
  echo "[P2-SANDBOX] changed-source marker scan FAILED"
  cat "$REPORT_DIR/static_markers.txt"
  exit 1
fi

(( fail == 0 ))
