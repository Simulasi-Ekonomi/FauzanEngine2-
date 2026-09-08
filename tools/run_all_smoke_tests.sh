#!/bin/bash
echo "=================================================="
echo "       NEOENGINE SUITE EXECUTION INTEGRITY        "
echo "=================================================="

BUILD_DIR="build"
PASS_COUNT=0
FAIL_COUNT=0

TESTS=(
    "r6_anti_cheat_100_smoke"
    "r7_economy_commerce_100_smoke"
    "r8_persistence_recovery_100_smoke"
    "r9_live_ops_telemetry_100_smoke"
    "vulkan_bootstrap_smoke"
    "bench_100k"
)

for test_bin in "${TESTS[@]}"; do
    if [ -f "$BUILD_DIR/$test_bin" ]; then
        echo -n "[RUN] $test_bin ... "
        ./"$BUILD_DIR/$test_bin" > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            echo "PASSED"
            ((PASS_COUNT++))
        else
            echo "FAILED"
            ((FAIL_COUNT++))
        fi
    else
        echo "[SKIP] $test_bin (Binary tidak ditemukan)"
    fi
done

echo "--------------------------------------------------"
echo "TOTAL PASSED: $PASS_COUNT | TOTAL FAILED: $FAIL_COUNT"
echo "=================================================="
