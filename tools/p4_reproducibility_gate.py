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
if a.stat().st_dev == b.stat().st_dev and a.stat().st_ino == b.stat().st_ino:
    raise SystemExit('P4_REPRODUCIBILITY_FAIL artifacts_must_be_distinct_files')
suffix_a = a.suffix.lower()
suffix_b = b.suffix.lower()
if suffix_a not in {'.apk', '.aab'} or suffix_b != suffix_a:
    raise SystemExit('P4_REPRODUCIBILITY_FAIL artifact_type_mismatch')
ha, hb = sha256(a), sha256(b)
if ha != hb:
    raise SystemExit(f'P4_REPRODUCIBILITY_FAIL sha256_a={ha} sha256_b={hb}')
print(f'P4_REPRODUCIBILITY_OK sha256={ha}')
