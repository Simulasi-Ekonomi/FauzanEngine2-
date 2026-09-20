#!/usr/bin/env bash
set -euo pipefail

MATRIX="release_readiness_matrix.md"
[[ -f "$MATRIX" ]] || { echo "P4_GATE: missing $MATRIX"; exit 1; }

required=(R1 R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12)
failures=0

for gate in "${required[@]}"; do
  line="$(grep -E "^\\| $gate \\|" "$MATRIX" || true)"
  if [[ -z "$line" ]]; then
    echo "P4_GATE: missing gate $gate"
    failures=1
    continue
  fi
  status="$(awk -F'\\|' '{gsub(/^ +| +$/, "", $NF); print $NF}' <<< "$line")"
  echo "$gate: $status"
  if [[ "$status" != "Passed" && "$status" != "Passed for Farm canonical tool scope" && "$status" != "Passed for canonical Farm vertical-slice scope" && "$status" != "Passed for canonical Farm renderer scope" && "$status" != "Passed for canonical input/audio lifecycle scope" && "$status" != "Passed for canonical Farm authoritative scope" ]]; then
    failures=1
  fi
done

if [[ "$failures" -ne 0 ]]; then
  echo "P4_GATE: NOT CERTIFIED — one or more mandatory release gates are incomplete."
  exit 1
fi

echo "P4_GATE: CERTIFIED"
