#!/usr/bin/env python3
"""Audit the canonical NeoEngine CMake source boundary.

The audit is deliberately fail-closed: every source named by the canonical
CMake list must exist, must be unique, and must not accidentally point into
legacy engine/ sources.  The report is deterministic so CI and local audits
can compare it directly.
"""
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


def parse_sources(cmake: Path) -> list[str]:
    text = cmake.read_text(encoding="utf-8")
    match = re.search(r"set\(XPBD_RUNTIME_SOURCES\s*(.*?)\n\)", text, re.S)
    if not match:
        raise SystemExit("XPBD_RUNTIME_SOURCES block not found")
    return re.findall(r"^\s+([^\s()]+\.cpp)\s*$", match.group(1), re.M)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument("--cmake", type=Path, default=Path("Source/NeoEngine/CMakeLists.txt"))
    parser.add_argument("--output", type=Path, default=Path("build/canonical-source-manifest.json"))
    args = parser.parse_args()

    root = args.root.resolve()
    cmake = (root / args.cmake).resolve()
    sources = parse_sources(cmake)
    duplicates = sorted({s for s in sources if sources.count(s) > 1})
    missing = sorted(s for s in sources if not (cmake.parent / s).is_file())
    legacy_refs = sorted(s for s in sources if s.startswith("../") or "/engine/" in s)

    manifest = {
        "schema": 1,
        "cmake": str(args.cmake).replace("\\", "/"),
        "source_count": len(sources),
        "sources": sources,
        "duplicates": duplicates,
        "missing": missing,
        "legacy_references": legacy_refs,
    }
    output = root / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    if duplicates or missing or legacy_refs:
        print(json.dumps(manifest, indent=2))
        return 1
    print(f"CANONICAL_SOURCE_AUDIT_OK sources={len(sources)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
