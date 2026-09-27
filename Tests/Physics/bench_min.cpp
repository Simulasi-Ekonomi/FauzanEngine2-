#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Threading/JobSystem.h"
#include <cstdio>
#include <memory>
using namespace NeoEngine;

int main() {
    printf("Init 8 workers...\n");
    JobSystem::Get().Initialize(8);
    printf("Create ECS & Physics...\n");
    ArchetypeManager em;
    auto phys = std::make_unique<XPBDPhysicsSystem>();
    const uint32_t f = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;
    printf("Create 500 entities...\n");
    for (int i = 0; i < 500; ++i) {
        EntityID id = em.CreateEntity(f);
        em.SetPosX(id, (i % 20) * 2.0f);
        em.SetPosZ(id, (i / 20) * 2.0f);
        em.SetVelX(id, 0.0f);
        em.SetVelZ(id, 0.0f);
        em.SetRadius(id, 0.8f);
        em.SetInvMass(id, 1.0f);
    }
    printf("Step 5 frames...\n");
    for (int i = 0; i < 5; ++i) {
        printf(" frame %d...\n", i);
        phys->Step(em, 0.016f);
        printf(" contacts: %zu\n", phys->GetManifoldCount());
    }
    printf("Done.\n");
    JobSystem::Get().Shutdown();
    return 0;
}
