#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-Build/p1_unreal_parity}"
ROOT="${2:-Source/NeoEngine}"
REPORT_DIR="${BUILD_DIR}/sandbox-report"
mkdir -p "$BUILD_DIR" "$REPORT_DIR"

echo "[P1-SANDBOX] configure"
cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
echo "[P1-SANDBOX] build canonical runtime"
cmake --build "$BUILD_DIR" -j2

mapfile -t TARGETS < <(
  sed -n 's/^[[:space:]]*add_xpbd_executable(\([^ )]*\).*/\1/p' "$ROOT/CMakeLists.txt" |
  awk '!seen[$0]++'
)

printf "%s\n" "${TARGETS[@]}" >"$REPORT_DIR/targets.txt"
echo "[P1-SANDBOX] discovered ${#TARGETS[@]} CMake smoke targets"
: > "$REPORT_DIR/execution.tsv"
pass=0
fail=0
skip=0

for target in "${TARGETS[@]}"; do
  if cmake --build "$BUILD_DIR" --target "$target" -j2 >/dev/null 2>&1; then
    exe="$BUILD_DIR/$target"
    if [[ -x "$exe" ]]; then
      if timeout 90s "$exe" >"$REPORT_DIR/$target.log" 2>&1; then
        printf '%s\tPASS\n' "$target" >> "$REPORT_DIR/execution.tsv"
        pass=$((pass+1))
      else
        rc=$?
        printf '%s\tFAIL\t%s\n' "$target" "$rc" >> "$REPORT_DIR/execution.tsv"
        fail=$((fail+1))
      fi
    else
      printf '%s\tSKIP\tno-executable\n' "$target" >> "$REPORT_DIR/execution.tsv"
      skip=$((skip+1))
    fi
  else
    printf '%s\tBUILD_FAIL\n' "$target" >> "$REPORT_DIR/execution.tsv"
    fail=$((fail+1))
  fi
done

echo "[P1-SANDBOX] static stub/placeholder scan"
if git grep -nE 'TODO|FIXME|XXX|placeholder|not implemented|IMPLEMENT_ME|return[[:space:]]*0;[[:space:]]*(//.*)?' --   ':(glob)Source/NeoEngine/**/*.cpp'   ':(glob)Source/NeoEngine/**/*.h' >"$REPORT_DIR/static_markers.txt"; then
  echo "[P1-SANDBOX] static markers found"
else
  : >"$REPORT_DIR/static_markers.txt"
  echo "[P1-SANDBOX] static marker scan clean"
fi

cat >"$REPORT_DIR/summary.txt" <<EOF
P1 Unreal-like parity sandbox
targets=$((pass+fail+skip))
pass=$pass
fail=$fail
skip=$skip
EOF

cat "$REPORT_DIR/summary.txt"
cat "$REPORT_DIR/execution.tsv"

if (( fail != 0 )); then
  exit 1
fi
