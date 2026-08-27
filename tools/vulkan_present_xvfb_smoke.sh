#!/usr/bin/env bash
# Runs an already-built Vulkan present smoke on a disposable virtual X11 display.
# This is not a device, user-window, GPU-performance, or production-rendering test.
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: $0 <cmake-build-directory>" >&2
    exit 2
fi

build_dir="$1"
smoke="$build_dir/vulkan_present_smoke"
command -v xvfb-run >/dev/null 2>&1 || { echo 'VULKAN_PRESENT_XVFB_SMOKE_FAIL reason=xvfb_missing' >&2; exit 2; }
test -x "$smoke" || { echo 'VULKAN_PRESENT_XVFB_SMOKE_FAIL reason=smoke_missing' >&2; exit 2; }
xvfb-run -a -s '-screen 0 1280x720x24' "$smoke"
echo 'VULKAN_PRESENT_XVFB_SMOKE_OK virtualX11=1'
