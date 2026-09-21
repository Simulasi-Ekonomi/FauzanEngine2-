#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import subprocess
import sys
from pathlib import Path

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

values = {line.split("=", 1)[0]: line.split("=", 1)[1] for line in lines[1:]}
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
if values["artifact_sha256"] != artifact_sha:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL artifact_hash_mismatch")
if values["reference_artifact_sha256"] != reference_sha:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL reference_artifact_hash_mismatch")
if values["manifest_sha256"] != manifest_sha:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL manifest_hash_mismatch")
if values["sbom_sha256"] != sbom_sha:
    raise SystemExit("P4_PROVENANCE_VERIFY_FAIL sbom_hash_mismatch")

print(f"P4_PROVENANCE_VERIFY_OK artifact={sys.argv[1]} commit={head}")
