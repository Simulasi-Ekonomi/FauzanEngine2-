#pragma once

#include <cstdint>

namespace NeoEngine {

struct PhysicsStepBudget final {
    // Capacity limits remain explicit so Accepts() can be used by schedulers.
    uint32_t maxBodies = 100000U;
    uint32_t maxCollisions = 200000U;
    // Target workload is a minimum: a benchmark must not pass by simulating less work.
    uint32_t minBodies = 100000U;
    uint32_t minCollisions = 200000U;
    // The production target is strictly below 5 ms, not <= 5 ms.
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
        return simulatedBodies >= minBodies &&
               simulatedBodies <= maxBodies &&
               collisionTests >= minCollisions &&
               collisionTests <= maxCollisions &&
               contactsSolved >= collisionTests &&
               elapsedMicroseconds < budgetMicroseconds;
    }
};

} // namespace NeoEngine
