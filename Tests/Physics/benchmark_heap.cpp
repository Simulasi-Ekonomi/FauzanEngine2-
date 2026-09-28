#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Threading/JobSystem.h"
#include <chrono>
#include <random>
#include <cstdio>
#include <memory>

using namespace NeoEngine;

void RunBenchmark(int entityCount, const char* label) {
    ArchetypeManager em;
    auto phys = std::make_unique<XPBDPhysicsSystem>();
    JobSystem::Get().Initialize(8);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> posDist(-50.0f, 50.0f);
    std::uniform_real_distribution<float> velDist(-2.0f, 2.0f);
    std::uniform_real_distribution<float> radiusDist(0.3f, 1.5f);

    const uint32_t flags = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;
    for (int i = 0; i < entityCount; ++i) {
        const EntityID id = em.CreateEntity(flags);
        const float radius = radiusDist(rng);
        em.SetPosX(id, posDist(rng));
        em.SetPosZ(id, posDist(rng));
        em.SetVelX(id, velDist(rng));
        em.SetVelZ(id, velDist(rng));
        em.SetRadius(id, radius);
        em.SetInvMass(id, 1.0f / (radius * 10.0f));
    }

    for (int f = 0; f < 30; ++f) phys->Step(em, 0.016f);

    constexpr int measFrames = 100;
    double totalMs = 0.0;
    size_t totalContacts = 0;
    for (int f = 0; f < measFrames; ++f) {
        const auto t1 = std::chrono::steady_clock::now();
        phys->Step(em, 0.016f);
        const auto t2 = std::chrono::steady_clock::now();
        totalMs += std::chrono::duration<double, std::milli>(t2 - t1).count();
        totalContacts += phys->GetManifoldCount();
    }

    const double avgMs = totalMs / measFrames;
    const double avgContacts = static_cast<double>(totalContacts) / measFrames;
    printf("Benchmark %s (%d entities):\n", label, entityCount);
    printf("  Avg frame time: %.2f ms\n", avgMs);
    printf("  Avg contacts  : %.0f\n", avgContacts);
    printf("  FPS (ideal)   : %.1f\n\n", 1000.0 / avgMs);

    JobSystem::Get().Shutdown();
}

int main() {
    printf("=== FAUZANENGINE V5.10 PHYSICS BENCHMARK ===\n\n");
    RunBenchmark(20000, "20K");
    RunBenchmark(50000, "50K");
    RunBenchmark(100000, "100K");
    return 0;
}
