#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Threading/JobSystem.h"

#include <cstdio>

using namespace NeoEngine;

int main() {
    JobSystem::Get().Initialize(4);

    ArchetypeManager entities;
    XPBDPhysicsSystem physics;
    const uint32_t flags = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;

    const EntityID dynamicA = entities.CreateEntity(flags);
    entities.SetPosX(dynamicA, 0.0f);
    entities.SetRadius(dynamicA, 0.35f);
    entities.SetInvMass(dynamicA, 1.0f);
    entities.SetVelX(dynamicA, 0.1f);

    const EntityID dynamicB = entities.CreateEntity(flags);
    entities.SetPosX(dynamicB, 0.45f);
    entities.SetRadius(dynamicB, 0.35f);
    entities.SetInvMass(dynamicB, 1.0f);

    const EntityID staticCollider = entities.CreateEntity(flags);
    entities.SetPosX(staticCollider, 0.85f);
    entities.SetRadius(staticCollider, 0.35f);
    entities.SetInvMass(staticCollider, 0.0f);

    const EntityID syncSentinel = entities.CreateEntity(flags);
    entities.SetPosX(syncSentinel, 100.0f);
    entities.SetRadius(syncSentinel, 0.20f);
    entities.SetInvMass(syncSentinel, 0.0f);

    // Dense-grid production path activates at 8,192 broadphase participants.
    // These separated movers preserve a single intentional local overlap while
    // ensuring the regression covers the same contact generation route as the
    // 100K benchmark.
    for (int index = 0; index < 8190; ++index) {
        const EntityID filler = entities.CreateEntity(flags);
        entities.SetPosX(filler, 10.0f + static_cast<float>(index));
        entities.SetRadius(filler, 0.20f);
        entities.SetInvMass(filler, 1.0f);
        entities.SetVelX(filler, 0.01f);
    }

    if (physics.AddDistanceJoint(dynamicA, dynamicB, 0.2f, 0.8f) == UINT32_MAX) {
        std::fprintf(stderr, "XPBD_REGRESSION_FAIL distance_joint\n");
        JobSystem::Get().Shutdown();
        return 1;
    }
    physics.AddClothPatch({{dynamicA, 0.0f, 0.0f, 1.0f}, {dynamicB, 1.0f, 0.0f, 1.0f}}, 1.0f, 0.0f);

    physics.Step(entities, 1.0f / 60.0f);
    const size_t contacts = physics.GetManifoldCount();
    physics.Step(entities, 1.0f / 60.0f);
    const auto overlaps = physics.OverlapSphere(0.45f, 0.0f, 1.0f);
    entities.SetPosX(syncSentinel, 200.0f);
    physics.Step(entities, 1.0f / 60.0f);
    bool externalSync = false;
    for (ArchetypeChunk* chunk : entities.GetChunks<PositionComponent, VelocityComponent, ColliderComponent>()) {
        if (!chunk || !chunk->entities || !chunk->posX) continue;
        for (size_t index = 0; index < chunk->count; ++index) {
            if (chunk->entities[index] == syncSentinel) externalSync = chunk->posX[index] == 200.0f;
        }
    }
    const auto serialized = physics.SerializePhysicsState();
    physics.DeserializePhysicsState(serialized);
    physics.EnableGPUBroadphase(true);
    physics.EnableGPUBroadphase(false);

    const bool ok = contacts > 0 && !overlaps.empty() && externalSync && !serialized.empty();
    if (ok) {
        std::printf("XPBD_REGRESSION_OK contacts=%zu bytes=%zu overlaps=%zu\n",
                    contacts, serialized.size(), overlaps.size());
    } else {
        std::fprintf(stderr, "XPBD_REGRESSION_FAIL contacts=%zu bytes=%zu overlaps=%zu external_sync=%d\n",
                     contacts, serialized.size(), overlaps.size(), externalSync ? 1 : 0);
    }

    JobSystem::Get().Shutdown();
    return ok ? 0 : 1;
}
