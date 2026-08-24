#!/bin/bash
echo "=== Struktur Direktori ==="
find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "CMakeLists.txt" \) | sort

echo ""
echo "=== File Versi / Penamaan Terkait V42 ==="
find . -type f -name "*42*" -o -name "*V42*" 2>/dev/null

echo ""
echo "=== Cek File Konfigurasi / Build ==="
cat CMakeLists.txt 2>/dev/null || echo "Tidak ada CMakeLists.txt di root"

echo ""
echo "=== Benchmark Terbaru (bench_20k.cpp) ==="
find . -path "*/bench_20k.cpp" -exec echo "File: {}" \; -exec cat {} \;

echo ""
echo "=== Header XPBDPhysicsSystem Saat Ini ==="
find . -name "XPBDPhysicsSystem.h" -exec echo "File: {}" \; -exec cat {} \;

echo ""
echo "=== Source XPBDPhysicsSystem Saat Ini ==="
find . -name "XPBDPhysicsSystem.cpp" -exec echo "File: {}" \; -exec cat {} \;

echo ""
echo "=== Isi File Lain (ArchetypeManager, SpatialGrid) ==="
find . -name "ArchetypeManager.h" -exec echo "File: {}" \; -exec head -50 {} \;
find . -name "SpatialGrid.h" -exec echo "File: {}" \; -exec head -50 {} \;
find . -name "SpatialGrid.cpp" -exec echo "File: {}" \; -exec head -50 {} \;

echo ""
echo "=== Selesai Audit ==="
