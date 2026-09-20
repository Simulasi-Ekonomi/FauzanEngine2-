#include "Runtime/CanonicalRuntimeWorld.h"

#include <cassert>
#include <cmath>

using namespace NeoEngine;

int main() {
    CanonicalRuntimeWorld world;

    CanonicalEntity sceneActor{};
    assert(world.CreateEntity({2.0F, 0.0F, 3.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F},
                              COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER,
                              CanonicalTransformAuthority::Scene, sceneActor));
    assert(sceneActor.hasECS);
    assert(world.ECS().HasEntity(sceneActor.ecs));
    assert(world.Scene().GetTransform(sceneActor.scene) != nullptr);

    assert(world.ConfigureReplication(ReplicationRole::Server, 7U, true));
    assert(world.RegisterReplicatedEntity(sceneActor, 42U, 7U));

    const uint64_t revisionBefore = world.ECS().GetPhysicsRevision();
    assert(world.Step(1.0F / 60.0F));
    assert(world.LastFrame().physicsStepped);
    assert(world.LastFrame().frame == 1U);
    assert(world.LastFrame().sceneEntities == 1U);
    assert(world.LastFrame().physicsEntities == 1U);
    assert(world.ECS().GetPhysicsRevision() >= revisionBefore);

    ReplicationSnapshot snapshot{};
    assert(world.BuildReplicationSnapshot(world.LastFrame().frame, snapshot));
    assert(snapshot.count == 1U);
    assert(snapshot.states[0].networkId == 42U);
    std::vector<uint8_t> snapshotBytes;
    ReplicationSnapshot decodedSnapshot{};
    assert(world.BuildReplicationSnapshot(world.LastFrame().frame, snapshot));

    const Transform3* sceneTransform = world.Scene().GetTransform(sceneActor.scene);
    assert(sceneTransform != nullptr);
    assert(std::isfinite(sceneTransform->x) && std::isfinite(sceneTransform->z));

    assert(world.SetTransform(sceneActor, {4.0F, 0.0F, -2.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}));
    assert(world.Step(1.0F / 60.0F));

    assert(world.UnregisterReplicatedEntity(42U));
    assert(world.DestroyEntity(sceneActor));
    assert(world.Scene().AliveCount() == 0U);
    return 0;
}
