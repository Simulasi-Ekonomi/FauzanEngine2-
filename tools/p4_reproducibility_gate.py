#!/usr/bin/env python3
import hashlib
import sys
from pathlib import Path

MAX_ARTIFACT_SIZE = 512 * 1024 * 1024
ALLOWED_SUFFIXES = {".apk", ".aab"}

def sha256(path: Path) -> tuple[str, int]:
    h = hashlib.sha256()
    initial_size = path.stat().st_size
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    final_size = path.stat().st_size
    if final_size != initial_size:
        raise SystemExit(f"P4_REPRODUCIBILITY_FAIL artifact_changed_during_hash={path}")
    return h.hexdigest(), final_size

if len(sys.argv) != 3:
    raise SystemExit("usage: tools/p4_reproducibility_gate.py <artifact-a> <artifact-b>")

raw_a, raw_b = (Path(value) for value in sys.argv[1:])
if any(ch in str(p) for p in (raw_a, raw_b) for ch in ("\x00", "\n", "\r")):
    raise SystemExit("P4_REPRODUCIBILITY_FAIL unsafe_artifact_path")

try:
    a = raw_a if raw_a.is_absolute() else Path.cwd() / raw_a
    b = raw_b if raw_b.is_absolute() else Path.cwd() / raw_b
    resolved_a = a.resolve(strict=True)
    resolved_b = b.resolve(strict=True)
except (OSError, RuntimeError) as exc:
    raise SystemExit(f"P4_REPRODUCIBILITY_FAIL unresolved_artifact_path={exc}") from exc

if resolved_a == resolved_b:
    raise SystemExit("P4_REPRODUCIBILITY_FAIL artifacts_must_be_distinct_paths")
if resolved_a.suffix.lower() not in ALLOWED_SUFFIXES or resolved_b.suffix.lower() != resolved_a.suffix.lower():
    raise SystemExit("P4_REPRODUCIBILITY_FAIL artifact_type_mismatch")

for p in (resolved_a, resolved_b):
    try:
        stat = p.stat()
    except OSError as exc:
        raise SystemExit(f"P4_REPRODUCIBILITY_FAIL artifact_stat_failed={p}") from exc
    if p.is_symlink() or not p.is_file() or stat.st_size == 0 or stat.st_size > MAX_ARTIFACT_SIZE:
        raise SystemExit(f"P4_REPRODUCIBILITY_FAIL invalid_or_oversized_artifact={p}")
    if p.suffix.lower() not in ALLOWED_SUFFIXES:
        raise SystemExit(f"P4_REPRODUCIBILITY_FAIL invalid_artifact={p}")

try:
    stat_a = resolved_a.stat()
    stat_b = resolved_b.stat()
except OSError as exc:
    raise SystemExit(f"P4_REPRODUCIBILITY_FAIL artifact_stat_failed={exc}") from exc
if stat_a.st_dev == stat_b.st_dev and stat_a.st_ino == stat_b.st_ino:
    raise SystemExit("P4_REPRODUCIBILITY_FAIL artifacts_must_be_distinct_files")

try:
    ha, size_a = sha256(resolved_a)
    hb, size_b = sha256(resolved_b)
except (OSError, PermissionError) as exc:
    raise SystemExit(f"P4_REPRODUCIBILITY_FAIL artifact_hash_failed={exc}") from exc
if size_a != stat_a.st_size or size_b != stat_b.st_size:
    raise SystemExit("P4_REPRODUCIBILITY_FAIL artifact_size_changed")
if ha != hb:
    raise SystemExit(f"P4_REPRODUCIBILITY_FAIL sha256_a={ha} sha256_b={hb}")

print(f"P4_REPRODUCIBILITY_OK sha256={ha}")
