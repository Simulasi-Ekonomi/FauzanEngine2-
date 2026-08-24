#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Threading/JobSystem.h"
#include <cstdio>
#include <memory>

using namespace NeoEngine;

int main() {
    JobSystem::Get().Initialize(4);
    ArchetypeManager em;
    auto phys = std::make_unique<XPBDPhysicsSystem>();
    const uint32_t flags = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;

    EntityID a = em.CreateEntity(flags);
    EntityID b = em.CreateEntity(flags);

    // Set posisi & radius
    em.SetPosX(a, 0.0f); em.SetPosZ(a, 0.0f);
    em.SetPosX(b, 0.3f); em.SetPosZ(b, 0.0f);
    em.SetRadius(a, 0.25f); em.SetRadius(b, 0.25f);
    em.SetInvMass(a, 1.0f); em.SetInvMass(b, 1.0f);

    // --- CEK CHUNK & ENTITY SEBELUM STEP ---
    auto chunks = em.GetChunks<PositionComponent, VelocityComponent, ColliderComponent>();
    printf("Jumlah chunk: %zu\n", chunks.size());
    size_t totalEntity = 0;
    for (auto* ch : chunks) {
        printf("  chunk mask=0x%x count=%zu\n", ch->mask, ch->count);
        totalEntity += ch->count;
    }
    printf("Total entity di chunk: %zu\n", totalEntity);
    printf("Radius via SoA: a=%f b=%f\n", em.GetRadius(a), em.GetRadius(b));

    // Step
    printf("Step...\n");
    phys->Step(em, 0.016f);
    printf("Contacts: %zu\n", phys->GetManifoldCount());

    JobSystem::Get().Shutdown();
    return 0;
}
