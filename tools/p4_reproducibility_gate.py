#!/usr/bin/env python3
import hashlib
import sys
from pathlib import Path

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()

if len(sys.argv) != 3:
    raise SystemExit('usage: tools/p4_reproducibility_gate.py <artifact-a> <artifact-b>')
a, b = map(Path, sys.argv[1:])
for p in (a, b):
    if not p.is_file() or p.is_symlink() or p.stat().st_size == 0:
        raise SystemExit(f'P4_REPRODUCIBILITY_FAIL invalid_artifact={p}')
if a.suffix not in {'.apk', '.aab'} or b.suffix != a.suffix:
    raise SystemExit('P4_REPRODUCIBILITY_FAIL artifact_type_mismatch')
ha, hb = sha256(a), sha256(b)
if ha != hb:
    raise SystemExit(f'P4_REPRODUCIBILITY_FAIL sha256_a={ha} sha256_b={hb}')
print(f'P4_REPRODUCIBILITY_OK sha256={ha}')
