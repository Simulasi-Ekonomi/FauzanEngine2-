#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MATRIX="$ROOT/release_readiness_matrix.md"

rows=$(grep -cE '^\| R(1|[2-9]|1[0-2]) \|' "$MATRIX")
passed=$(grep -cE '^\| R(2|3|4|5|6|7|8|9|10|11|12) .*\| Not passed \|' "$MATRIX")
r1_scope=$(grep -cF '| Passed for Farm canonical tool scope |' "$MATRIX")

[[ "$rows" -eq 12 ]]
[[ "$passed" -eq 11 ]]
[[ "$r1_scope" -eq 1 ]]
printf 'LAUNCH_READINESS_GUARD_SMOKE_OK gates=%s blocked=%s release_ready=0 owner_signoff=missing\n' "$rows" "$passed"
