#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import subprocess
from pathlib import Path

root = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip())
if not root.is_dir() or root.is_symlink():
    raise SystemExit("SOURCE_SBOM_FAIL invalid_repository_root")
files = subprocess.check_output(
    ["git", "ls-files", "-z", ":!*.keystore", ":!*.jks", ":!*.p12", ":!build/", ":!out/", ":!dist/"],
    text=False,
).split(b"\0")

components = []
tracked_paths = sorted(set(raw.decode("utf-8", "strict") for raw in files if raw))
if any("\x00" in path or path.startswith("/") or ".." in Path(path).parts for path in tracked_paths):
    raise SystemExit("SOURCE_SBOM_FAIL unsafe_tracked_path")
for path in tracked_paths:
    full = root / path
    if not full.is_file() or full.is_symlink():
        continue
    if not full.resolve().is_relative_to(root.resolve()):
        raise SystemExit("SOURCE_SBOM_FAIL path_escape")
    digest = hashlib.sha256(full.read_bytes()).hexdigest()
    components.append({
        "type": "file",
        "bom-ref": f"file:{path}",
        "name": path,
        "version": "source",
        "hashes": [{"alg": "SHA-256", "content": digest}],
    })

bom = {
    "bomFormat": "CycloneDX",
    "specVersion": "1.5",
    "version": 1,
    "metadata": {
        "component": {
            "type": "application",
            "name": "FauzanEngine2-",
            "version": subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
        }
    },
    "components": components,
}
output = root / "p4-source-sbom.json"
if output.exists() and not output.is_file():
    raise SystemExit("SOURCE_SBOM_FAIL output_not_regular")
if output.is_symlink():
    raise SystemExit("SOURCE_SBOM_FAIL output_symlink")
output.write_text(json.dumps(bom, indent=2, sort_keys=True) + "\n", encoding="utf-8")
print(f"SOURCE_SBOM_OK files={len(components)}")
