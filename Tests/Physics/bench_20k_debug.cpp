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
    printf("=== DEBUG HANG ===\n");
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
    
    printf("Creating %d entities...\n", N);
    for (int i = 0; i < N; ++i) {
        EntityID id = em.CreateEntity(flags);
        em.SetPosX(id, (i % COLS) * SPACING + jitter(rng));
        em.SetPosZ(id, (i / COLS) * SPACING + jitter(rng));
        em.SetVelX(id, vel(rng)); em.SetVelZ(id, vel(rng));
        em.SetRadius(id, 0.25f);
        em.SetInvMass(id, 1.0f / (0.25f*0.25f*3.14159f*10.0f));
    }
    printf("Warmup 5 frames (debug)...\n");
    for (int i = 0; i < 5; ++i) {
        printf("  frame %d start...", i); fflush(stdout);
        phys->Step(em, 0.016f);
        printf(" done. contacts=%zu, hash=%zu\n", phys->GetManifoldCount(), phys->GetSlotUsed());
    }
    printf("OK\n");
    JobSystem::Get().Shutdown();
    return 0;
}
