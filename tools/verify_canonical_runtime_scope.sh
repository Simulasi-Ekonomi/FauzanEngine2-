#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
engine_root="$repo_root/Source/NeoEngine"
cmake_file="$engine_root/CMakeLists.txt"
manifest="$repo_root/tools/canonical_runtime_scope_manifest_v1.txt"

die() {
    printf 'canonical-runtime-scope: FAIL: %s\n' "$1" >&2
    exit 1
}

[[ -f "$cmake_file" ]] || die "missing canonical CMake source list"
[[ -f "$manifest" ]] || die "missing scope manifest"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

awk '
    /^set\(XPBD_RUNTIME_SOURCES[[:space:]]*$/ { inside = 1; next }
    inside && /^[[:space:]]*\)[[:space:]]*$/ { exit }
    inside {
        sub(/#.*/, "")
        gsub(/^[[:space:]]+|[[:space:]]+$/, "")
        if ($0 != "") print $0
    }
' "$cmake_file" | sort -u > "$tmp_dir/active_sources"

[[ -s "$tmp_dir/active_sources" ]] || die "XPBD_RUNTIME_SOURCES could not be parsed"

awk -F'|' '
    /^[[:space:]]*#/ || /^[[:space:]]*$/ { next }
    NF != 2 || $1 == "" || $2 == "" { exit 2 }
    { print $1 "|" $2 }
' "$manifest" > "$tmp_dir/manifest_entries" || die "malformed scope manifest"

cut -d'|' -f2 "$tmp_dir/manifest_entries" | sort > "$tmp_dir/manifest_paths"
[[ "$(wc -l < "$tmp_dir/manifest_paths")" -eq "$(sort -u "$tmp_dir/manifest_paths" | wc -l)" ]] || die "duplicate manifest path"

while IFS='|' read -r classification path; do
    [[ -f "$engine_root/$path" ]] || die "manifest file missing: $path"
    if [[ "$classification" == active_fail_closed_legacy ]]; then
        grep -Fxq "$path" "$tmp_dir/active_sources" || die "active classified path omitted from XPBD_RUNTIME_SOURCES: $path"
    elif [[ "$classification" == active_enum_marker ]]; then
        companion_source="${path%.h}.cpp"
        grep -Fxq "$companion_source" "$tmp_dir/active_sources" || die "active enum marker has no active companion source: $path"
    else
        if grep -Fxq "$path" "$tmp_dir/active_sources"; then
            die "non-active classified source was admitted to XPBD_RUNTIME_SOURCES: $path"
        fi
    fi
done < "$tmp_dir/manifest_entries"

grep -Fq 'NOT_IMPLEMENTED: legacy EngineLoop' "$engine_root/Core/EngineLoop.cpp" || die "legacy EngineLoop is not explicit fail-closed"
grep -Fq 'use NeoRuntime for canonical state execution' "$engine_root/Core/EngineLoop.cpp" || die "legacy EngineLoop does not direct callers to NeoRuntime"
grep -Fxq 'Runtime/NeoRuntime.cpp' "$tmp_dir/active_sources" || die "NeoRuntime is not active"
grep -Fxq 'Runtime/VulkanPresentProbe.cpp' "$tmp_dir/active_sources" || die "VulkanPresentProbe is not active"
grep -Fxq 'Runtime/FarmRuntimeSession.cpp' "$tmp_dir/active_sources" || die "FarmRuntimeSession is not active"

find "$engine_root" -type f \( -name '*.h' -o -name '*.cpp' \) -print0 \
    | xargs -0 grep -IlE 'TODO|FIXME|NOT_IMPLEMENTED|NotImplemented|placeholder|Placeholder|stub|Stub' \
    | sed "s#^$engine_root/##" \
    | sort > "$tmp_dir/observed_marker_paths"

if ! diff -u "$tmp_dir/manifest_paths" "$tmp_dir/observed_marker_paths"; then
    die "unclassified, stale, or removed capability-marker path"
fi

active_marker_count=0
while IFS='|' read -r classification path; do
    case "$classification" in
        active_fail_closed_legacy)
            grep -Fxq "$path" "$tmp_dir/active_sources" && active_marker_count=$((active_marker_count + 1))
            ;;
        active_enum_marker)
            companion_source="${path%.h}.cpp"
            grep -Fxq "$companion_source" "$tmp_dir/active_sources" && active_marker_count=$((active_marker_count + 1))
            ;;
        *)
            if grep -Fxq "$path" "$tmp_dir/active_sources"; then
                die "unapproved active marker classification: $classification ($path)"
            fi
            ;;
    esac
done < "$tmp_dir/manifest_entries"

printf 'canonical-runtime-scope: PASS: %s active sources; %s tracked marker paths; %s approved active markers\n' \
    "$(wc -l < "$tmp_dir/active_sources")" \
    "$(wc -l < "$tmp_dir/manifest_paths")" \
    "$active_marker_count"
