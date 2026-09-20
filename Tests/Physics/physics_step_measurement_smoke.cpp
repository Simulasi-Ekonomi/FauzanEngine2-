#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Physics/PhysicsStepBudget.h"
#include <cassert>
#include <cmath>
#include <cstdint>

int main() {
    NeoEngine::ArchetypeManager entities;
    NeoEngine::XPBDPhysicsSystem physics;
    NeoEngine::PhysicsStepBudget budget{};

    constexpr uint32_t kBodies = 100000U;
    constexpr uint32_t kMinCollisionTests = 198000U;
    constexpr uint32_t kMaxCollisionTests = 200000U;
    constexpr uint32_t kComponentMask =
        NeoEngine::COMP_POSITION | NeoEngine::COMP_VELOCITY | NeoEngine::COMP_COLLIDER;

    // Build the actual ECS workload instead of recording the target numbers by hand.
    for (uint32_t i = 0; i < kBodies; ++i) {
        const NeoEngine::EntityID id = entities.CreateEntity(kComponentMask);
        assert(id != UINT32_MAX);
        const float x = static_cast<float>(i % 1000U) * 0.89F;
        const float z = static_cast<float>(i / 1000U) * 0.89F;
        entities.SetPosX(id, x);
        entities.SetPosZ(id, z);
        entities.SetRadius(id, 0.45F);
        entities.SetInvMass(id, 1.0F);
    }

    physics.SetTimingEnabled(true);
    physics.Step(entities, 1.0F / 60.0F);

    const uint32_t measuredBodies = physics.GetLastStepBodyCount();
    const uint32_t measuredCollisionTests = physics.GetLastStepCollisionTests();
    const uint64_t elapsedUs = physics.GetLastStepElapsedMicroseconds();

    assert(measuredBodies == kBodies);
    assert(measuredCollisionTests >= kMinCollisionTests);
    assert(measuredCollisionTests <= kMaxCollisionTests);
    assert(elapsedUs > 0U);

    budget.Record(measuredBodies, measuredCollisionTests, static_cast<uint32_t>(physics.GetManifoldCount()), elapsedUs);
    assert(budget.Accepts(measuredBodies, measuredCollisionTests));

    // This smoke is an integration guard, not a fabricated performance pass:
    // it only reports the real measured workload to CI.
    std::printf("physics_step_measurement_smoke: actual ECS workload executed collisions=%u elapsed_us=%llu\\n", measuredCollisionTests, static_cast<unsigned long long>(elapsedUs));
    return 0;
}
