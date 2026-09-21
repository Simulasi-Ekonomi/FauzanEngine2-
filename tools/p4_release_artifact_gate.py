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

    raw_path = str(args.artifact)
    if not raw_path.strip() or "\x00" in raw_path or "\n" in raw_path or "\r" in raw_path:
        raise SystemExit("P4_ARTIFACT_GATE_FAIL unsafe_artifact_path")
    if args.artifact.is_symlink():
        raise SystemExit("P4_ARTIFACT_GATE_FAIL symlink_artifact_path")
    artifact = args.artifact.resolve()
    if not artifact.is_file():
        raise SystemExit(f"P4_ARTIFACT_GATE_FAIL missing_or_symlink={args.artifact}")
    if artifact.suffix.lower() not in {".apk", ".aab"}:
        raise SystemExit("P4_ARTIFACT_GATE_FAIL unsupported_artifact_type")
    if not artifact.is_absolute():
        raise SystemExit("P4_ARTIFACT_GATE_FAIL artifact_path_not_absolute")

    artifact_size = artifact.stat().st_size
    if artifact_size == 0:
        raise SystemExit("P4_ARTIFACT_GATE_FAIL empty_artifact")
    if artifact_size > 512 * 1024 * 1024:
        raise SystemExit("P4_ARTIFACT_GATE_FAIL artifact_too_large")

    try:
        with zipfile.ZipFile(artifact) as archive:
            if archive.testzip() is not None:
                raise SystemExit("P4_ARTIFACT_GATE_FAIL corrupt_zip")
            names = archive.namelist()
            if len(names) > 100000:
                raise SystemExit("P4_ARTIFACT_GATE_FAIL too_many_zip_entries")
            if any("\x00" in name or "\n" in name or "\r" in name for name in names):
                raise SystemExit("P4_ARTIFACT_GATE_FAIL malformed_zip_name")
            if len(names) != len(set(names)):
                raise SystemExit("P4_ARTIFACT_GATE_FAIL duplicate_zip_entries")
            required_entries = {"AndroidManifest.xml"} if artifact.suffix.lower() == ".apk" else {"base/manifest/AndroidManifest.xml", "BundleConfig.pb"}
            missing_entries = sorted(required_entries.difference(names))
            if missing_entries:
                raise SystemExit("P4_ARTIFACT_GATE_FAIL missing_required_entries=" + ",".join(missing_entries))
            for required_entry in required_entries:
                info = archive.getinfo(required_entry)
                if info.is_dir() or info.file_size == 0:
                    raise SystemExit(f"P4_ARTIFACT_GATE_FAIL invalid_required_entry={required_entry}")
            total_uncompressed = 0
            for info in archive.infolist():
                name = info.filename
                if "\x00" in name or "\\" in name or name.startswith("/") or ".." in Path(name).parts or name.startswith("./"):
                    raise SystemExit("P4_ARTIFACT_GATE_FAIL unsafe_zip_path")
                if info.flag_bits & 0x1:
                    raise SystemExit("P4_ARTIFACT_GATE_FAIL encrypted_zip_entry")
                if info.flag_bits & (1 << 5 | 1 << 6):
                    raise SystemExit("P4_ARTIFACT_GATE_FAIL unsupported_zip_flags")
                if info.flag_bits & ((1 << 13) | (1 << 14) | (1 << 15)):
                    raise SystemExit("P4_ARTIFACT_GATE_FAIL reserved_or_encrypted_zip_flags")
                if len(name) > 1024:
                    raise SystemExit("P4_ARTIFACT_GATE_FAIL zip_name_too_long")
                if info.file_size > 256 * 1024 * 1024:
                    raise SystemExit("P4_ARTIFACT_GATE_FAIL oversized_zip_entry")
                if info.compress_size == 0 and info.file_size > 0:
                    raise SystemExit("P4_ARTIFACT_GATE_FAIL invalid_zip_compression_size")
                total_uncompressed += info.file_size
                if total_uncompressed > 1024 * 1024 * 1024:
                    raise SystemExit("P4_ARTIFACT_GATE_FAIL uncompressed_payload_too_large")
                if info.compress_size > 0 and info.file_size / info.compress_size > 200.0:
                    raise SystemExit("P4_ARTIFACT_GATE_FAIL suspicious_compression_ratio")
                # A release archive must never contain a POSIX symlink entry.
                if info.compress_type not in {zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED, zipfile.ZIP_BZIP2, zipfile.ZIP_LZMA}:
                    raise SystemExit("P4_ARTIFACT_GATE_FAIL unsupported_zip_compression")
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
