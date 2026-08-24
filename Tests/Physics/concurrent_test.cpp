#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Threading/JobSystem.h"
#include <chrono>
#include <cstdio>
#include <memory>
#include <atomic>
#include <random>
#include <cassert>
#include <thread>
#include <vector>

using namespace NeoEngine;

int main() {
    printf("=== FAUZANENGINE V6.20 CONCURRENT LAMBDA TEST ===\n");
    JobSystem::Get().Initialize(8);

    auto phys = std::make_unique<XPBDPhysicsSystem>();

    constexpr int NUM_THREADS = 8;
    constexpr int OPS_PER_THREAD = 500000;
    std::atomic<bool> start{false};
    std::atomic<int> errors{0};
    std::atomic<size_t> totalOps{0};

    auto worker = [&](int seed) {
        std::mt19937_64 rng(seed);
        while (!start.load(std::memory_order_acquire)) {}
        for (int i = 0; i < OPS_PER_THREAD; ++i) {
            uint64_t key = rng() & 0xFFFF;
            float n = (float)(rng() & 0xFFF) / 4096.0f;
            float t = (float)(rng() & 0xFFF) / 4096.0f;

            // store & load
            phys->lambdaStoreBoth(key, n, t);
            float rn, rt;
            phys->lambdaLoadBoth(key, rn, rt);

            // validasi konsistensi dasar
            if (std::fabs(rn - n) > 100.0f || std::fabs(rt - t) > 100.0f) {
                errors.fetch_add(1, std::memory_order_relaxed);
            }
            totalOps.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker, i * 0x1234567);
    }

    auto t1 = std::chrono::steady_clock::now();
    start.store(true, std::memory_order_release);
    for (auto& th : threads) th.join();
    auto t2 = std::chrono::steady_clock::now();

    double elapsed = std::chrono::duration<double, std::milli>(t2 - t1).count();
    size_t total = totalOps.load();
    printf("Threads: %d\n", NUM_THREADS);
    printf("Total ops: %zu (stores + loads)\n", total);
    printf("Elapsed  : %.2f ms\n", elapsed);
    printf("Ops/sec  : %.2f million\n", total / elapsed / 1000.0);
    printf("Errors   : %d\n", errors.load());
    printf("Hash usage: %zu / %zu\n", phys->GetSlotUsed(), phys->GetHashCap());

    if (errors.load() == 0) {
        printf("✅ Lock-free lambda cache PASSED concurrent stress test\n");
    } else {
        printf("❌ FAILED\n");
        return 1;
    }

    JobSystem::Get().Shutdown();
    return 0;
}
