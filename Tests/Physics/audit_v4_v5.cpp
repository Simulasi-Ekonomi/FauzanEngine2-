#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Core/ECS/ArchetypeManager.h"
#include <chrono>
#include <iostream>
#include <vector>

using namespace NeoEngine;

int main() {
    constexpr uint32_t N = 10000;
    constexpr int frames = 50;
    constexpr float dt = 1.0f / 60.0f;
    constexpr uint32_t mask = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;

    ArchetypeManager world;
    for (uint32_t i = 0; i < N; ++i) {
        const EntityID id = world.CreateEntity(mask);
        const float x = static_cast<float>((i % 100) * 0.39f);
        const float z = static_cast<float>((i / 100) * 0.39f);
        world.SetPosX(id, x);
        world.SetPosZ(id, z);
        world.SetVelX(id, static_cast<float>((i % 7) - 3) * 0.01f);
        world.SetVelZ(id, static_cast<float>((i % 5) - 2) * 0.01f);
        world.SetRadius(id, 0.2f);
        world.SetInvMass(id, 1.0f);
    }

    XPBDPhysicsSystem physics;
    physics.SetTimingEnabled(true);
    physics.SetProbeMetricsEnabled(true);

    const auto started = std::chrono::steady_clock::now();
    for (int frame = 0; frame < frames; ++frame) {
        physics.Step(world, dt);
    }
    const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    const double avgMs = elapsed / frames;
    const auto& broadphase = physics.GetBroadphaseStats();
    const auto& timing = physics.GetStepTimingStats();

    std::cout << "============ XPBD V5 AUDIT ============\n";
    std::cout << "Entities: " << N << " | Frames: " << frames << "\n";
    std::cout << "Average step: " << avgMs << " ms\n";
    std::cout << "Last step: " << timing.totalMs << " ms\n";
    std::cout << "Candidate pairs: " << broadphase.candidatePairs << "\n";
    std::cout << "Manifolds: " << physics.GetManifoldCount() << "\n";
    if (broadphase.candidatePairs == 0 || physics.GetManifoldCount() == 0) {
        std::cerr << "XPBD V5 audit produced no collision workload.\n";
        return 2;
    }
    return 0;
}
