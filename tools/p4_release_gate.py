#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
from pathlib import Path

ROOT = Path(subprocess.check_output(
    ["git", "rev-parse", "--show-toplevel"], text=True
).strip())

required_files = [
    ROOT / "tools" / "release_manifest.sh",
    ROOT / "tools" / "generate_source_sbom.py",
    ROOT / ".github" / "workflows" / "p4-release-manifest.yml",
    ROOT / ".github" / "workflows" / "p4-source-sbom.yml",
]
for path in required_files:
    if not path.is_file():
        raise SystemExit(f"P4_RELEASE_GATE_FAIL missing={path.relative_to(ROOT)}")

for pattern in ("*.keystore", "*.jks", "*.p12"):
    leaked = [
        p for p in ROOT.rglob(pattern)
        if ".git" not in p.parts and "build" not in p.parts and "out" not in p.parts
    ]
    if leaked:
        raise SystemExit(
            "P4_RELEASE_GATE_FAIL secret_material="
            + ",".join(str(p.relative_to(ROOT)) for p in leaked)
        )

manifest = ROOT / "p4-release-manifest.sha256"
sbom = ROOT / "p4-source-sbom.json"
if not manifest.is_file():
    raise SystemExit("P4_RELEASE_GATE_FAIL missing=generated release manifest")
if not sbom.is_file():
    raise SystemExit("P4_RELEASE_GATE_FAIL missing=generated source SBOM")

lines = manifest.read_text(encoding="utf-8").splitlines()
if len(lines) < 4 or lines[0] != "FAUZANENGINE_RELEASE_MANIFEST_V1":
    raise SystemExit("P4_RELEASE_GATE_FAIL invalid manifest header")
if not all(lines[i].split("=", 1)[0] in {"commit", "tree", "files"} for i in range(1, 4)):
    raise SystemExit("P4_RELEASE_GATE_FAIL invalid manifest metadata")

bom = json.loads(sbom.read_text(encoding="utf-8"))
if bom.get("bomFormat") != "CycloneDX" or bom.get("specVersion") != "1.5":
    raise SystemExit("P4_RELEASE_GATE_FAIL invalid SBOM format")
if not bom.get("components"):
    raise SystemExit("P4_RELEASE_GATE_FAIL empty SBOM")

print(f"P4_RELEASE_GATE_OK files={len(bom['components'])}")
