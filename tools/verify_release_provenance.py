#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import subprocess
import sys
from pathlib import Path

def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while True:
            chunk = stream.read(1024 * 1024)
            if not chunk:
                break
            digest.update(chunk)
    return digest.hexdigest()

ROOT = Path(subprocess.check_output(
    ["git", "rev-parse", "--show-toplevel"], text=True
).strip())

if len(sys.argv) != 2:
    raise SystemExit("usage: tools/verify_release_provenance.py <release.apk|release.aab>")

artifact = Path(sys.argv[1])
if not artifact.is_absolute():
    artifact = ROOT / artifact
if artifact.is_symlink():
    raise SystemExit('P4_PROVENANCE_VERIFY_FAIL artifact_symlink')
artifact = artifact.resolve(strict=True)
provenance = ROOT / "p4-release-provenance.txt"
manifest_path = ROOT / "p4-release-manifest.sha256"
sbom_path = ROOT / "p4-source-sbom.json"

if artifact.stat().st_size == 0:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL empty_artifact")
if not artifact.is_file():
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL artifact_not_regular_file")
if artifact.suffix not in {".apk", ".aab"}:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL unsupported_artifact_extension")
if not provenance.is_file() or provenance.is_symlink():
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL missing_provenance")

lines = provenance.read_text(encoding="utf-8").splitlines()
expected_keys = [
    "FAUZANENGINE_RELEASE_PROVENANCE_V1",
    "commit=",
    "tree=",
    "artifact=",
    "artifact_sha256=",
    "reference_artifact=",
    "reference_artifact_sha256=",
    "manifest_sha256=",
    "sbom_sha256=",
]
if len(lines) != len(expected_keys) or any(not line.startswith(prefix) for line, prefix in zip(lines, expected_keys)):
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL malformed_provenance")
if len(set(line.split("=", 1)[0] for line in lines[1:])) != len(expected_keys) - 1:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL duplicate_provenance_key")
if any("=" not in line for line in lines[1:]):
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL malformed_provenance_field")
if any(len(line.split("=", 1)[1]) > 4096 for line in lines[1:]):
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL oversized_provenance_field")

values = {line.split("=", 1)[0]: line.split("=", 1)[1] for line in lines[1:]}

if any(len(values[key]) != 40 or any(ch not in "0123456789abcdef" for ch in values[key].lower()) for key in ("commit", "tree")):
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL malformed_git_identity")
if any(len(values[key]) != 64 or any(ch not in "0123456789abcdef" for ch in values[key].lower()) for key in ("artifact_sha256", "reference_artifact_sha256", "manifest_sha256", "sbom_sha256")):
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL malformed_sha256")
head = subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
tree = subprocess.check_output(["git", "rev-parse", "HEAD^{tree}"], text=True).strip()
if values["commit"] != head or values["tree"] != tree:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL git_identity_mismatch")

if any(ord(ch) < 32 for ch in values["artifact"]):
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL unsafe_artifact_identity")
if values["artifact"] != sys.argv[1]:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL artifact_identity_mismatch")
reference = Path(values["reference_artifact"])
if not reference.is_absolute():
    reference = ROOT / reference
if reference.is_symlink():
    raise SystemExit('P4_PROVENANCE_VERIFY_FAIL reference_artifact_symlink')
reference = reference.resolve(strict=True)
if not reference.is_file() or reference.suffix != artifact.suffix:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL invalid_reference_artifact")
if values["reference_artifact"] == sys.argv[1] or artifact == reference:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL reference_artifact_identity_mismatch")

if not manifest_path.is_file() or manifest_path.is_symlink() or not sbom_path.is_file() or sbom_path.is_symlink():
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL missing_or_symlink_attestation")
artifact_sha = file_sha256(artifact)
reference_sha = file_sha256(reference)
manifest_sha = file_sha256(manifest_path)
sbom_sha = file_sha256(sbom_path)
manifest_lines = manifest_path.read_text(encoding="utf-8").splitlines()
if len(manifest_lines) < 4 or manifest_lines[0] != "FAUZANENGINE_RELEASE_MANIFEST_V1":
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL malformed_manifest")
manifest_values = {}
for line in manifest_lines[1:4]:
    if "=" not in line:
        raise SystemExit("P4_PROVENANCE_VERIFY_FAIL malformed_manifest_header")
    key, value = line.split("=", 1)
    if key in manifest_values:
        raise SystemExit("P4_PROVENANCE_VERIFY_FAIL duplicate_manifest_header")
    manifest_values[key] = value
if manifest_values.get("commit") != values["commit"] or manifest_values.get("tree") != values["tree"]:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL manifest_git_identity_mismatch")
try:
    manifest_count = int(manifest_values.get("files", "-1"))
except ValueError:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL malformed_manifest_count")
manifest_entries = manifest_lines[4:]
if manifest_count != len(manifest_entries) or manifest_count < 0:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL manifest_count_mismatch")
for entry in manifest_entries:
    parts = entry.split("  ", 1)
    if len(parts) != 2 or len(parts[0]) != 64 or any(ch not in "0123456789abcdef" for ch in parts[0]) or not parts[1].strip():
        raise SystemExit("P4_PROVENANCE_VERIFY_FAIL malformed_manifest_entry")
try:
    sbom = json.loads(sbom_path.read_text(encoding="utf-8"))
except (OSError, json.JSONDecodeError):
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL malformed_sbom")
if sbom.get("bomFormat") != "CycloneDX" or sbom.get("specVersion") != "1.5" or sbom.get("version") != 1:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL invalid_sbom_format")
if sbom.get("metadata", {}).get("component", {}).get("type") != "application" or sbom.get("metadata", {}).get("component", {}).get("version") != values["commit"]:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL sbom_git_identity_mismatch")
components = sbom.get("components")
if not isinstance(components, list) or len(components) != len({item.get("bom-ref") for item in components if isinstance(item, dict)}):
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL malformed_sbom_components")
for component in components:
    if not isinstance(component, dict) or component.get("type") != "file" or component.get("version") != "source":
        raise SystemExit("P4_PROVENANCE_VERIFY_FAIL malformed_sbom_component")
    hashes = component.get("hashes")
    if not isinstance(hashes, list) or not hashes or hashes[0].get("alg") != "SHA-256" or len(hashes[0].get("content", "")) != 64:
        raise SystemExit("P4_PROVENANCE_VERIFY_FAIL malformed_sbom_digest")
if values["artifact_sha256"] != artifact_sha:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL artifact_hash_mismatch")
if values["reference_artifact_sha256"] != reference_sha:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL reference_artifact_hash_mismatch")
if values["manifest_sha256"] != manifest_sha:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL manifest_hash_mismatch")
if values["sbom_sha256"] != sbom_sha:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL sbom_hash_mismatch")

print(f"P4_PROVENANCE_VERIFY_OK artifact={sys.argv[1]} commit={head}")
