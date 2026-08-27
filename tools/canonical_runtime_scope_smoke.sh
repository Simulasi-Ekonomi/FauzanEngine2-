#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
probe_path="$repo_root/Source/NeoEngine/Runtime/CanonicalRuntimeScopeAdversarialProbe.cpp"
verifier="$repo_root/tools/verify_canonical_runtime_scope.sh"

cleanup() {
    rm -f "$probe_path"
}
trap cleanup EXIT INT TERM

[[ -x "$verifier" ]] || {
    printf 'canonical-runtime-scope-smoke: FAIL: verifier is not executable\n' >&2
    exit 1
}

"$verifier"
printf '%s\n' '// TODO: adversarial unclassified marker probe' > "$probe_path"

if "$verifier" > /dev/null 2>&1; then
    printf 'canonical-runtime-scope-smoke: FAIL: unclassified marker was accepted\n' >&2
    exit 1
fi

cleanup
trap - EXIT INT TERM
"$verifier"
printf 'canonical-runtime-scope-smoke: PASS: unclassified marker rejected and clean scope restored\n'
