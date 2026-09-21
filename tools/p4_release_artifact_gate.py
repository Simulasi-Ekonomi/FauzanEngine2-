#!/usr/bin/env python3
from __future__ import annotations
import argparse
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

MAX_ARTIFACT_SIZE=512*1024*1024
MAX_ENTRY_SIZE=256*1024*1024
MAX_TOTAL_UNCOMPRESSED=1024*1024*1024
MAX_ENTRIES=100000
MAX_ARCHIVE_COMMENT=65535
MAX_PATH_DEPTH=32
MAX_EXTRA_FIELD=65535
MAX_NAME_BYTES=1024
MAX_COMPRESSION_RATIO=200.0
MAX_ENTRY_COMMENT=65535
ALLOWED_SUFFIXES={".apk",".aab"}

def run_checked(command:list[str],label:str)->None:
    try: result=subprocess.run(command,text=True,capture_output=True,check=False)
    except OSError as exc: raise SystemExit(f"P4_ARTIFACT_GATE_FAIL {label}=tool_unavailable") from exc
    if result.returncode!=0:
        detail=(result.stdout+result.stderr).strip().replace("\n"," ")
        raise SystemExit(f"P4_ARTIFACT_GATE_FAIL {label}=verification_failed detail={detail[:512]}")

def main()->int:
    parser=argparse.ArgumentParser(description="Verify a release APK/AAB before certification.")
    parser.add_argument("artifact",type=Path);args=parser.parse_args()
    raw_path=str(args.artifact)
    raw_parts=Path(raw_path).parts
    if ".." in raw_parts: raise SystemExit("P4_ARTIFACT_GATE_FAIL parent_artifact_path")
    if not raw_path.strip() or any(ch in raw_path for ch in ("\x00","\n","\r")): raise SystemExit("P4_ARTIFACT_GATE_FAIL unsafe_artifact_path")
    if args.artifact.is_symlink(): raise SystemExit("P4_ARTIFACT_GATE_FAIL symlink_artifact_path")
    try: artifact=args.artifact.resolve(strict=True)
    except (OSError,RuntimeError) as exc: raise SystemExit(f"P4_ARTIFACT_GATE_FAIL unresolved_artifact_path={exc}") from exc
    if not artifact.is_file() or artifact.is_symlink() or not artifact.exists(): raise SystemExit(f"P4_ARTIFACT_GATE_FAIL missing_or_symlink={args.artifact}")
    if not artifact.is_absolute() or artifact.suffix.lower() not in ALLOWED_SUFFIXES: raise SystemExit("P4_ARTIFACT_GATE_FAIL invalid_artifact_path")
    try: artifact_size=artifact.stat().st_size
    except OSError as exc: raise SystemExit(f"P4_ARTIFACT_GATE_FAIL artifact_stat_failed={artifact}") from exc
    if artifact_size<=0 or artifact_size>MAX_ARTIFACT_SIZE: raise SystemExit("P4_ARTIFACT_GATE_FAIL invalid_artifact_size")
    try:
        with zipfile.ZipFile(artifact) as archive:
            if archive.testzip() is not None: raise SystemExit("P4_ARTIFACT_GATE_FAIL corrupt_zip")
            if len(archive.comment)>MAX_ARCHIVE_COMMENT: raise SystemExit("P4_ARTIFACT_GATE_FAIL oversized_archive_comment")
            names=archive.namelist()
            if len(names)==0 or len(names)>MAX_ENTRIES: raise SystemExit("P4_ARTIFACT_GATE_FAIL too_many_zip_entries")
            if len(names)!=len(set(names)): raise SystemExit("P4_ARTIFACT_GATE_FAIL duplicate_zip_entries")
            if any(not name or "\x00" in name or "\n" in name or "\r" in name or name.strip()!=name or len(name.encode("utf-8"))>65535 for name in names): raise SystemExit("P4_ARTIFACT_GATE_FAIL malformed_zip_name")
            required_entries={"AndroidManifest.xml"} if artifact.suffix.lower()==".apk" else {"base/manifest/AndroidManifest.xml","BundleConfig.pb"}
            missing=sorted(required_entries.difference(names))
            if missing: raise SystemExit("P4_ARTIFACT_GATE_FAIL missing_required_entries="+",".join(missing))
            for required in required_entries:
                try:
                    if archive.getinfo(required).file_size <= 0: raise SystemExit("P4_ARTIFACT_GATE_FAIL empty_required_entry="+required)
                except KeyError as exc: raise SystemExit("P4_ARTIFACT_GATE_FAIL missing_required_entry="+required) from exc
            total_uncompressed=0
            previous_entry_end=0
            for info in archive.infolist():
                name=info.filename
                if "\\" in name or name.startswith("/") or name.startswith("./") or Path(name).is_absolute() or ".." in Path(name).parts: raise SystemExit("P4_ARTIFACT_GATE_FAIL unsafe_zip_path")
                if len(name.encode("utf-8"))>MAX_NAME_BYTES or len(Path(name).parts)>MAX_PATH_DEPTH or any(len(part)>255 for part in Path(name).parts) or name.endswith("/../"): raise SystemExit("P4_ARTIFACT_GATE_FAIL zip_name_too_long")
                if any(ord(ch)<0x20 or ord(ch)==0x7f for ch in name): raise SystemExit("P4_ARTIFACT_GATE_FAIL control_character_zip_name")
                if len(info.extra)>MAX_EXTRA_FIELD: raise SystemExit("P4_ARTIFACT_GATE_FAIL oversized_zip_extra")
                if info.flag_bits & 0x1 or info.flag_bits & ((1<<5)|(1<<6)|(1<<13)|(1<<14)|(1<<15)): raise SystemExit("P4_ARTIFACT_GATE_FAIL unsafe_zip_flags")
                if info.header_offset < 0 or info.header_offset >= artifact_size: raise SystemExit("P4_ARTIFACT_GATE_FAIL invalid_zip_header_offset")
                header_end=info.header_offset+30+len(name.encode("utf-8"))+len(info.extra)
                if header_end>artifact_size or header_end<info.header_offset: raise SystemExit("P4_ARTIFACT_GATE_FAIL invalid_local_header_extent")
                data_end=header_end+info.compress_size
                if data_end>artifact_size or data_end<header_end: raise SystemExit("P4_ARTIFACT_GATE_FAIL invalid_compressed_data_extent")
                if info.header_offset<previous_entry_end: raise SystemExit("P4_ARTIFACT_GATE_FAIL overlapping_zip_entries")
                previous_entry_end=data_end
                if len(info.comment)>MAX_ENTRY_COMMENT: raise SystemExit("P4_ARTIFACT_GATE_FAIL oversized_entry_comment")
                if info.file_size>MAX_ENTRY_SIZE or info.compress_size>MAX_ENTRY_SIZE or info.file_size<0 or info.compress_size<0 or info.volume!=0: raise SystemExit("P4_ARTIFACT_GATE_FAIL invalid_zip_volume")
                if info.file_size>MAX_ENTRY_SIZE or info.compress_size>MAX_ENTRY_SIZE or info.file_size<0 or info.compress_size<0 or info.header_offset + info.compress_size > artifact_size: raise SystemExit("P4_ARTIFACT_GATE_FAIL oversized_zip_entry")
                if info.compress_size==0 and info.file_size>0: raise SystemExit("P4_ARTIFACT_GATE_FAIL invalid_zip_compression_size")
                if total_uncompressed > MAX_TOTAL_UNCOMPRESSED - info.file_size: raise SystemExit("P4_ARTIFACT_GATE_FAIL uncompressed_payload_too_large")
                total_uncompressed+=info.file_size
                if total_uncompressed>MAX_TOTAL_UNCOMPRESSED: raise SystemExit("P4_ARTIFACT_GATE_FAIL uncompressed_payload_too_large")
                if info.compress_size>0 and info.file_size/info.compress_size>MAX_COMPRESSION_RATIO: raise SystemExit("P4_ARTIFACT_GATE_FAIL suspicious_compression_ratio")
                if info.compress_type not in {zipfile.ZIP_STORED,zipfile.ZIP_DEFLATED,zipfile.ZIP_BZIP2,zipfile.ZIP_LZMA}: raise SystemExit("P4_ARTIFACT_GATE_FAIL unsupported_zip_compression")
                if info.create_system == 3 and ((info.external_attr >> 16) & 0o170000) not in {0,0o100000,0o040000,0o120000}: raise SystemExit("P4_ARTIFACT_GATE_FAIL special_zip_entry")
                if info.create_system == 3 and ((info.external_attr >> 16) & 0o170000) == 0o120000: raise SystemExit("P4_ARTIFACT_GATE_FAIL symlink_zip_entry")
                if info.is_dir() and not name.endswith("/"):
                    raise SystemExit("P4_ARTIFACT_GATE_FAIL malformed_directory_entry")
                if info.is_dir() and info.file_size != 0: raise SystemExit("P4_ARTIFACT_GATE_FAIL nonzero_directory_size")
                if info.is_dir() and info.compress_size != 0: raise SystemExit("P4_ARTIFACT_GATE_FAIL compressed_directory_entry")
                if info.reserved != 0 or info.flag_bits & 0x800 == 0 and any(ord(ch)>127 for ch in name): raise SystemExit("P4_ARTIFACT_GATE_FAIL non_utf8_name_flag")
                if info.reserved != 0: raise SystemExit("P4_ARTIFACT_GATE_FAIL reserved_zip_field")
                if info.is_dir() and not name.endswith("/"): raise SystemExit("P4_ARTIFACT_GATE_FAIL malformed_directory_entry")
                if not info.is_dir() and name.endswith("/"): raise SystemExit("P4_ARTIFACT_GATE_FAIL malformed_file_entry")
                if not info.is_dir() and info.file_size==0 and name in required_entries: raise SystemExit("P4_ARTIFACT_GATE_FAIL empty_required_entry")
    except zipfile.BadZipFile as exc: raise SystemExit("P4_ARTIFACT_GATE_FAIL invalid_zip") from exc
    if total_uncompressed == 0: raise SystemExit("P4_ARTIFACT_GATE_FAIL empty_uncompressed_archive")
    if artifact.suffix.lower()==".apk":
        if artifact_size < 64U: raise SystemExit("P4_ARTIFACT_GATE_FAIL artifact_too_small_for_zip")
        apksigner=shutil.which("apksigner")
        if apksigner is None: raise SystemExit("P4_ARTIFACT_GATE_FAIL missing_tool=apksigner")
        run_checked([apksigner,"verify","--verbose","--print-certs",str(artifact)],"apk_signature")
        run_checked([apksigner,"verify","--min-sdk-version","1",str(artifact)],"apk_min_sdk_verification")
    else:
        jarsigner=shutil.which("jarsigner")
        if jarsigner is None: raise SystemExit("P4_ARTIFACT_GATE_FAIL missing_tool=jarsigner")
        run_checked([jarsigner,"-verify","-strict",str(artifact)],"aab_signature")
        run_checked([jarsigner,"-verify","-strict","-certs",str(artifact)],"aab_certificate_verification")
    print(f"P4_ARTIFACT_GATE_OK artifact={artifact}")
    return 0
if __name__=="__main__": sys.exit(main())
