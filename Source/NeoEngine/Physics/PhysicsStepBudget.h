#pragma once

#include <cstdint>

namespace NeoEngine {

struct PhysicsStepBudget final {
    uint32_t maxBodies = 100000U;
    uint32_t maxCollisions = 200000U;
    uint32_t budgetMicroseconds = 5000U;
    uint32_t simulatedBodies = 0U;
    uint32_t collisionTests = 0U;
    uint32_t contactsSolved = 0U;
    uint64_t elapsedMicroseconds = 0U;

    bool Accepts(uint32_t bodies, uint32_t collisions) const {
        return bodies <= maxBodies && collisions <= maxCollisions;
    }

    void Record(uint32_t bodies, uint32_t collisions, uint32_t contacts, uint64_t elapsedUs) {
        simulatedBodies = bodies;
        collisionTests = collisions;
        contactsSolved = contacts;
        elapsedMicroseconds = elapsedUs;
    }

    bool MeetsTarget() const {
        return simulatedBodies <= maxBodies && collisionTests <= maxCollisions && elapsedMicroseconds <= budgetMicroseconds;
    }
};

} // namespace NeoEngine
