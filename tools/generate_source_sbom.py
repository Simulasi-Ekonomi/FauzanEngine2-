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
tracked_paths = sorted(set(raw.decode() for raw in files if raw))
for path in tracked_paths:
    full = root / path
    if not full.is_file() or full.is_symlink():
        continue
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
if output.is_symlink():
    raise SystemExit("SOURCE_SBOM_FAIL output_symlink")
output.write_text(json.dumps(bom, indent=2, sort_keys=True) + "\n", encoding="utf-8")
print(f"SOURCE_SBOM_OK files={len(components)}")
