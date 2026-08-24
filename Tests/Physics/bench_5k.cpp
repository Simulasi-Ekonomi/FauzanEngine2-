#include "Core/ECS/ArchetypeManager.h"
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
    printf("=== V12.0 BENCHMARK 5000 ENTITY ===\n");
    JobSystem::Get().Initialize(8);
    
    ArchetypeManager em;
    auto phys = std::make_unique<XPBDPhysicsSystem>();
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> jitter(-0.02f, 0.02f);
    std::uniform_real_distribution<float> vel(-0.5f, 0.5f);
    
    const uint32_t flags = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;
    constexpr int N = 5000;
    constexpr float SPACING = 0.35f;
    constexpr int COLS = 70;
    
    printf("Creating %d entities...\n", N);
    for (int i = 0; i < N; ++i) {
        EntityID id = em.CreateEntity(flags);
        em.SetPosX(id, (i % COLS) * SPACING + jitter(rng));
        em.SetPosZ(id, (i / COLS) * SPACING + jitter(rng));
        em.SetVelX(id, vel(rng)); em.SetVelZ(id, vel(rng));
        em.SetRadius(id, 0.25f);
        em.SetInvMass(id, 1.0f / (0.25f*0.25f*3.14159f*10.0f));
    }
    
    printf("Warmup 5 frames...\n");
    for (int i = 0; i < 5; ++i) phys->Step(em, 0.016f);
    
    printf("Bench 20 frames:\n");
    double sum = 0;
    for (int i = 0; i < 20; ++i) {
        auto t1 = std::chrono::steady_clock::now();
        phys->Step(em, 0.016f);
        auto t2 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        sum += ms;
        printf("  Frame %d: %.2f ms, contacts=%zu\n", i, ms, phys->GetManifoldCount());
    }
    printf("Avg: %.2f ms\n", sum/20.0);
    JobSystem::Get().Shutdown();
    return 0;
}
