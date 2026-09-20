#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import subprocess
import re
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

for pattern in ("*.keystore", "*.jks", "*.p12", "*-release-key", "*-signing-key"):
    leaked = [
        p for p in ROOT.rglob(pattern)
        if ".git" not in p.parts and "build" not in p.parts and "out" not in p.parts
    ]
    if leaked:
        raise SystemExit(
            "P4_RELEASE_GATE_FAIL secret_material="
            + ",".join(str(p.relative_to(ROOT)) for p in leaked)
        )

for forbidden_name in ("gradle.properties", "local.properties"):
    leaked = [
        p for p in ROOT.rglob(forbidden_name)
        if ".git" not in p.parts and "build" not in p.parts and "out" not in p.parts
    ]
    if leaked:
        raise SystemExit(
            "P4_RELEASE_GATE_FAIL local_signing_config="
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
manifest_commit = lines[1].split("=", 1)[1]
manifest_tree = lines[2].split("=", 1)[1]
try:\n    manifest_files = int(lines[3].split("=", 1)[1])\nexcept ValueError as exc:\n    raise SystemExit("P4_RELEASE_GATE_FAIL malformed manifest file count") from exc\nif manifest_files < 0:\n    raise SystemExit("P4_RELEASE_GATE_FAIL negative manifest file count")
head_commit = subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
head_tree = subprocess.check_output(["git", "rev-parse", "HEAD^{tree}"], text=True).strip()
if not re.fullmatch(r"[0-9a-f]{40}", manifest_commit) or not re.fullmatch(r"[0-9a-f]{40}", manifest_tree):
    raise SystemExit("P4_RELEASE_GATE_FAIL malformed git identity")
if manifest_commit != head_commit or manifest_tree != head_tree:
    raise SystemExit("P4_RELEASE_GATE_FAIL manifest does not describe current HEAD")
hash_lines = lines[4:]
if len(hash_lines) != manifest_files:
    raise SystemExit("P4_RELEASE_GATE_FAIL manifest file count mismatch")
for line in hash_lines:
    parts = line.split("  ", 1)
    if len(parts) != 2 or not re.fullmatch(r"[0-9a-f]{64}", parts[0]):
        raise SystemExit("P4_RELEASE_GATE_FAIL malformed manifest hash entry")
    expected, relative = parts
    path = ROOT / relative
    if not path.is_file():
        raise SystemExit(f"P4_RELEASE_GATE_FAIL missing_manifest_file={relative}")
    if path.is_symlink():
        raise SystemExit(f"P4_RELEASE_GATE_FAIL symlink_manifest_file={relative}")
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual != expected:
        raise SystemExit(f"P4_RELEASE_GATE_FAIL hash_mismatch={relative}")

bom = json.loads(sbom.read_text(encoding="utf-8"))
if bom.get("bomFormat") != "CycloneDX" or bom.get("specVersion") != "1.5":
    raise SystemExit("P4_RELEASE_GATE_FAIL invalid SBOM format")
components = bom.get("components")
if not components:
    raise SystemExit("P4_RELEASE_GATE_FAIL empty SBOM")

manifest_entries = {}
for line in hash_lines:
    expected, relative = line.split("  ", 1)
    if relative in manifest_entries:
        raise SystemExit(f"P4_RELEASE_GATE_FAIL duplicate manifest path={relative}")
    manifest_entries[relative] = expected
sbom_entries = {}
for component in components:
    if component.get("type") != "file" or not component.get("name"):
        raise SystemExit("P4_RELEASE_GATE_FAIL malformed SBOM component")
    name = component["name"]
    normalized = Path(name)
    if normalized.is_absolute() or ".." in normalized.parts or str(normalized) != name:
        raise SystemExit(f"P4_RELEASE_GATE_FAIL unsafe SBOM path={name}")
    if name in sbom_entries:
        raise SystemExit(f"P4_RELEASE_GATE_FAIL duplicate SBOM path={name}")
    hashes = component.get("hashes") or []
    sha = next((item.get("content") for item in hashes if item.get("alg") == "SHA-256"), None)
    if not sha or len(sha) != 64:
        raise SystemExit("P4_RELEASE_GATE_FAIL missing SBOM SHA-256")
    sbom_entries[name] = sha
if sbom_entries != manifest_entries:
    raise SystemExit("P4_RELEASE_GATE_FAIL SBOM does not exactly match release manifest")

print(f"P4_RELEASE_GATE_OK files={len(bom['components'])}")
