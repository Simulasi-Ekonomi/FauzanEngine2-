#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Threading/JobSystem.h"
#include <cstdio>
#include <memory>

using namespace NeoEngine;

int main() {
    printf("=== DEBUG 1 ===\n");
    JobSystem::Get().Initialize(8);
    printf("=== DEBUG 2 ===\n");
    ArchetypeManager em;
    printf("=== DEBUG 3 ===\n");
    auto phys = std::make_unique<XPBDPhysicsSystem>();
    printf("=== DEBUG 4 ===\n");
    
    const uint32_t flags = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;
    for (int i = 0; i < 1000; ++i) {
        EntityID id = em.CreateEntity(flags);
        em.SetPosX(id, i * 0.1f);
        em.SetPosZ(id, i * 0.1f);
        em.SetVelX(id, 0.0f);
        em.SetVelZ(id, 0.0f);
        em.SetRadius(id, 0.25f);
        em.SetInvMass(id, 1.0f);
    }
    printf("=== DEBUG 5 ===\n");
    phys->Step(em, 0.016f);
    printf("=== DEBUG 6 === contacts=%zu\n", phys->GetManifoldCount());
    JobSystem::Get().Shutdown();
    return 0;
}
