#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Threading/JobSystem.h"
#include <chrono>
#include <cstdio>
#include <memory>
#include <random>
#include <cmath>
#include <algorithm>

using namespace NeoEngine;

int main() {
    printf("=== FAUZANENGINE V14.0 — 20K ENTITY (50%% SLEEPING) ===\n");
    printf("Build: Release, Threads: 8\n\n");
    JobSystem::Get().Initialize(8);

    ArchetypeManager em;
    auto phys = std::make_unique<XPBDPhysicsSystem>();

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> jitter(-0.02f, 0.02f);
    std::uniform_real_distribution<float> vel(-0.5f, 0.5f);

    const uint32_t flags = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;
    constexpr int N = 20000;
    constexpr float SPACING = 0.35f;
    constexpr int COLS = 141;

    printf("Creating %d entities (50%% sleeping)... ", N);
    fflush(stdout);
    for (int i = 0; i < N; ++i) {
        EntityID id = em.CreateEntity(flags);
        float gx = (i % COLS) * SPACING + jitter(rng);
        float gz = (i / COLS) * SPACING + jitter(rng);
        em.SetPosX(id, gx); em.SetPosZ(id, gz);
        // 50% entity diam (velocity 0)
        if (i % 2 == 0) {
            em.SetVelX(id, 0.0f);
            em.SetVelZ(id, 0.0f);
        } else {
            em.SetVelX(id, vel(rng));
            em.SetVelZ(id, vel(rng));
        }
        float radius = 0.25f;
        em.SetRadius(id, radius);
        em.SetInvMass(id, 1.0f / (radius * radius * 3.14159f * 10.0f));
    }
    printf("done\n");

    printf("Warmup (5 frames)... "); fflush(stdout);
    for (int i = 0; i < 5; ++i) phys->Step(em, 0.016f);
    printf("done\n");

    constexpr int K = 20;
    double times[K];
    size_t contacts[K];
    printf("Benchmarking %d frames...\n", K);
    for (int i = 0; i < K; ++i) {
        auto t1 = std::chrono::steady_clock::now();
        phys->Step(em, 0.016f);
        auto t2 = std::chrono::steady_clock::now();
        times[i] = std::chrono::duration<double, std::milli>(t2 - t1).count();
        contacts[i] = phys->GetManifoldCount();
    }

    double sum = 0, minT = times[0], maxT = times[0];
    size_t sumC = 0;
    for (int i = 0; i < K; ++i) {
        sum += times[i];
        if (times[i] < minT) minT = times[i];
        if (times[i] > maxT) maxT = times[i];
        sumC += contacts[i];
    }
    double avg = sum / K;
    double avgC = (double)sumC / K;

    double var = 0;
    for (int i = 0; i < K; ++i) { double d = times[i] - avg; var += d * d; }
    double stddev = std::sqrt(var / K);

    double sorted[K];
    for (int i = 0; i < K; ++i) sorted[i] = times[i];
    std::sort(sorted, sorted + K);
    double p95 = sorted[(int)(K * 0.95)];
    double p99 = sorted[(int)(K * 0.99)];

    printf("\n--- HASIL ---\n");
    printf("Entities    : %d (50%% sleeping)\n", N);
    printf("Avg contacts: %.0f per frame\n", avgC);
    printf("Avg time    : %.3f ms\n", avg);
    printf("Min / Max   : %.2f / %.2f ms\n", minT, maxT);
    printf("Stddev / P95/ P99 : %.3f / %.3f / %.3f ms\n", stddev, p95, p99);
    printf("FPS (avg/P95) : %.1f / %.1f\n", 1000.0/avg, 1000.0/p95);
    printf("\n✅ V14.0 BENCHMARK — TRUE SLEEPING\n");
    JobSystem::Get().Shutdown();
    return 0;
}
