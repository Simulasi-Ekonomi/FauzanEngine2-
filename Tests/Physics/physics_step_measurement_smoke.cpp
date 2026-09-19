#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Physics/PhysicsStepBudget.h"
#include <cassert>
int main() {
    NeoEngine::ArchetypeManager entities;
    NeoEngine::XPBDPhysicsSystem physics;
    NeoEngine::PhysicsStepBudget budget{};
    budget.Record(100000U, 200000U, 0U, physics.GetLastStepElapsedMicroseconds());
    assert(budget.Accepts(100000U, 200000U));
    assert(!budget.Accepts(100001U, 200000U));
    physics.Step(entities, 1.0F / 60.0F);
    assert(physics.GetLastStepBodyCount() == 0U);
    assert(physics.GetLastStepElapsedMicroseconds() == 0U);
    return 0;
}
