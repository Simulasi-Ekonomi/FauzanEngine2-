#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path


def run_checked(command: list[str], label: str) -> None:
    try:
        result = subprocess.run(command, text=True, capture_output=True, check=False)
    except OSError as exc:
        raise SystemExit(f"P4_ARTIFACT_GATE_FAIL {label}=tool_unavailable") from exc
    if result.returncode != 0:
        detail = (result.stdout + result.stderr).strip().replace("\n", " ")
        raise SystemExit(f"P4_ARTIFACT_GATE_FAIL {label}=verification_failed detail={detail[:512]}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify a release APK/AAB before certification.")
    parser.add_argument("artifact", type=Path)
    args = parser.parse_args()

    artifact = args.artifact.resolve()
    if not artifact.is_file() or artifact.is_symlink():
        raise SystemExit(f"P4_ARTIFACT_GATE_FAIL missing_or_symlink={args.artifact}")
    if artifact.suffix.lower() not in {".apk", ".aab"}:
        raise SystemExit("P4_ARTIFACT_GATE_FAIL unsupported_artifact_type")

    if artifact.stat().st_size == 0:
        raise SystemExit("P4_ARTIFACT_GATE_FAIL empty_artifact")

    try:
        with zipfile.ZipFile(artifact) as archive:
            if archive.testzip() is not None:
                raise SystemExit("P4_ARTIFACT_GATE_FAIL corrupt_zip")
            names = archive.namelist()
            if len(names) != len(set(names)):
                raise SystemExit("P4_ARTIFACT_GATE_FAIL duplicate_zip_entries")
            for info in archive.infolist():
                name = info.filename
                if "\x00" in name or "\\" in name or name.startswith("/") or ".." in Path(name).parts:
                    raise SystemExit("P4_ARTIFACT_GATE_FAIL unsafe_zip_path")
                # A release archive must never contain a POSIX symlink entry.
                if (info.external_attr >> 16) & 0o170000 == 0o120000:
                    raise SystemExit("P4_ARTIFACT_GATE_FAIL symlink_zip_entry")
    except zipfile.BadZipFile as exc:
        raise SystemExit("P4_ARTIFACT_GATE_FAIL invalid_zip") from exc

    if artifact.suffix.lower() == ".apk":
        apksigner = shutil.which("apksigner")
        if apksigner is None:
            raise SystemExit("P4_ARTIFACT_GATE_FAIL missing_tool=apksigner")
        run_checked([apksigner, "verify", "--verbose", "--print-certs", str(artifact)], "apk_signature")
    else:
        jarsigner = shutil.which("jarsigner")
        if jarsigner is None:
            raise SystemExit("P4_ARTIFACT_GATE_FAIL missing_tool=jarsigner")
        run_checked([jarsigner, "-verify", "-strict", str(artifact)], "aab_signature")

    print(f"P4_ARTIFACT_GATE_OK artifact={artifact}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
