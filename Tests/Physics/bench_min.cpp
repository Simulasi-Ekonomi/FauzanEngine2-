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
        auto* p = em.GetPosition(id);
        auto* v = em.GetVelocity(id);
        auto* c = em.GetCollider(id);
        p->x = (i % 20) * 2.0f; p->z = (i / 20) * 2.0f;
        v->vx = 0; v->vz = 0;
        c->radius = 0.8f; c->invMass = 1.0f;
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
