#!/bin/bash
cd "/sdcard/Buku saya/FauzanEngine"

# Kompilasi benchmark langsung (tanpa libneo_core.so)
# Ambil hanya file sumber yang diperlukan untuk fisika
g++ -std=c++23 -O3 -march=armv8-a+simd \
    -I Source/NeoEngine \
    -I Source/NeoEngine/Core \
    -I Source/NeoEngine/Core/ECS \
    -I Source/NeoEngine/AI \
    -I Source/NeoEngine/Physics \
    -I Source/NeoEngine/Physics/V5 \
    -I Source/NeoEngine/World \
    -I Source/NeoEngine/Systems \
    -I Source/NeoEngine/Threading \
    -I Source/NeoEngine/Memory \
    Tests/Physics/benchmark.cpp \
    Source/NeoEngine/Core/ECS/ArchetypeManager.cpp \
    Source/NeoEngine/Core/ECS/EntityManager.cpp \
    Source/NeoEngine/Physics/V5/XPBDPhysicsSystem.cpp \
    Source/NeoEngine/Threading/JobSystem.cpp \
    Source/NeoEngine/World/SpatialGrid.cpp \
    Source/NeoEngine/Core/Memory/PoolAllocator.cpp \
    Source/NeoEngine/Core/Memory/FrameAllocator.cpp \
    -o build/neo_bench_standalone \
    -lpthread 2>&1

if [ -f "build/neo_bench_standalone" ]; then
    echo "✅ Standalone benchmark built. Running..."
    ./build/neo_bench_standalone
else
    echo "❌ Build failed"
fi
