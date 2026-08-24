#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Threading/JobSystem.h"
#include <chrono>
#include <cstdio>
#include <memory>
#include <random>
using namespace NeoEngine;

void test(int N) {
    printf("--- N=%d ---\n", N);
    fflush(stdout);

    JobSystem::Get().Initialize(8);
    ArchetypeManager em;
    auto phys = std::make_unique<XPBDPhysicsSystem>();

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> jitter(-0.02f, 0.02f);
    std::uniform_real_distribution<float> vel(-0.5f, 0.5f);
    const uint32_t flags = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;
    constexpr float SPACING = 0.35f;
    constexpr int COLS = 141;

    for (int i = 0; i < N; ++i) {
        EntityID id = em.CreateEntity(flags);
        float gx = (i % COLS) * SPACING + jitter(rng);
        float gz = (i / COLS) * SPACING + jitter(rng);
        em.SetPosX(id, gx); em.SetPosZ(id, gz);
        em.SetVelX(id, vel(rng)); em.SetVelZ(id, vel(rng));
        em.SetRadius(id, 0.25f);
        em.SetInvMass(id, 1.0f / (0.25f * 0.25f * 3.14159f * 10.0f));
    }

    printf("Warmup 5 frames... "); fflush(stdout);
    for (int i = 0; i < 5; ++i) {
        phys->Step(em, 0.016f);
    }
    printf("OK, contacts=%zu\n", phys->GetManifoldCount());
    fflush(stdout);

    JobSystem::Get().Shutdown();
}

int main() {
    test(10);
    test(100);
    test(1000);
    test(2000);
    test(5000);
    printf("✅ Scale test selesai\n");
    return 0;
}
