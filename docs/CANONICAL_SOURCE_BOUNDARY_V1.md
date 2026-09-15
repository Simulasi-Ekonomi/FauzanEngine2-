# Canonical Source Boundary v1

The production C++ route is the source set declared by `Source/NeoEngine/CMakeLists.txt` in `XPBD_RUNTIME_SOURCES` plus the explicitly registered executable smoke targets.

`tools/canonical_source_audit.py` is the authoritative boundary check. It fails when a canonical source is missing, duplicated, or references the legacy `engine/` tree. It also emits a deterministic JSON manifest for audit records.

## Local gate

```bash
python3 tools/canonical_source_audit.py
```

Expected output:

```text
CANONICAL_SOURCE_AUDIT_OK sources=<count>
```

A green result means the declared CMake boundary is internally consistent. It does **not** promote inactive files into runtime capability and does not claim release readiness.

## Classification rule

- `Source/NeoEngine` + active CMake source list: canonical runtime boundary.
- `Source/NeoEngine` outside that list: inactive until deliberately migrated and wired into a canonical target.
- `engine/`: legacy/out of scope for canonical runtime claims.
- A smoke target proves only the behavior it actually executes.

This boundary is intentionally independent of feature implementation so readiness audits cannot infer capability merely from file presence.
