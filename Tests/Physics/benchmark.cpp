#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Threading/JobSystem.h"
#include <chrono>
#include <random>
#include <cstdio>
#include <vector>

using namespace NeoEngine;

void RunBenchmark(int entityCount, const char* label) {
    // Init engine systems
    ArchetypeManager em;
    XPBDPhysicsSystem phys;
    JobSystem::Get().Initialize(8);   // 8 worker threads

    // Create entities randomly in a 100x100 area
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> posDist(-50.0f, 50.0f);
    std::uniform_real_distribution<float> velDist(-2.0f, 2.0f);
    std::uniform_real_distribution<float> radiusDist(0.3f, 1.5f);

    const uint32_t flags = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;
    for (int i = 0; i < entityCount; ++i) {
        EntityID id = em.CreateEntity(flags);
        auto* pos = em.GetPosition(id);
        auto* vel = em.GetVelocity(id);
        auto* col = em.GetCollider(id);
        pos->x = posDist(rng);
        pos->z = posDist(rng);
        vel->vx = velDist(rng);
        vel->vz = velDist(rng);
        col->radius = radiusDist(rng);
        col->invMass = 1.0f / (col->radius * 10.0f);  // heavier bigger objects
    }

    // Warmup
    for (int f = 0; f < 30; ++f) {
        phys.Step(em, 0.016f);
    }

    // Benchmark
    constexpr int measFrames = 100;
    double totalMs = 0.0;
    size_t totalContacts = 0;
    for (int f = 0; f < measFrames; ++f) {
        auto t1 = std::chrono::steady_clock::now();
        phys.Step(em, 0.016f);
        auto t2 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        totalMs += ms;
        totalContacts += phys.GetManifoldCount();
    }

    double avgMs = totalMs / measFrames;
    double avgContacts = (double)totalContacts / measFrames;
    printf("Benchmark %s (%d entities):\n", label, entityCount);
    printf("  Avg frame time: %.2f ms\n", avgMs);
    printf("  Avg contacts  : %.0f\n", avgContacts);
    printf("  FPS (ideal)   : %.1f\n", 1000.0 / avgMs);
    printf("\n");

    JobSystem::Get().Shutdown();
}

int main() {
    printf("=== FAUZANENGINE V5.10 PHYSICS BENCHMARK ===\n\n");
    RunBenchmark(20000, "20K");
    RunBenchmark(50000, "50K");
    RunBenchmark(100000, "100K");
    return 0;
}
