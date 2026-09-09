#include "Core/ECS/ArchetypeManager.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

int main() {
    using namespace NeoEngine;

    ArchetypeManager manager;
    const EntityID first = manager.CreateEntity(COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER);
    assert(first != UINT32_MAX);

    for (std::size_t i = 0; i < 1024; ++i) {
        assert(manager.CreateEntity(COMP_POSITION) != UINT32_MAX);
    }

    const EntityID second = manager.CreateEntity(COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER);
    assert(second != UINT32_MAX);

    manager.SetPosX(first, 11.0f);
    manager.SetPosZ(second, 22.0f);

    const auto chunks = manager.GetChunks<PositionComponent>();
    assert(chunks.size() >= 2);

    bool foundFirst = false;
    bool foundSecond = false;
    for (const auto* chunk : chunks) {
        for (std::size_t i = 0; i < chunk->count; ++i) {
            if (chunk->entities[i] == first) {
                assert(chunk->posX != nullptr && chunk->posX[i] == 11.0f);
                foundFirst = true;
            }
            if (chunk->entities[i] == second) {
                assert(chunk->posZ != nullptr && chunk->posZ[i] == 22.0f);
                foundSecond = true;
            }
        }
    }
    assert(foundFirst && foundSecond);

    const std::uint64_t revision = manager.GetPhysicsRevision();
    manager.SetPosX(999999u, 7.0f);
    assert(manager.GetPhysicsRevision() == revision);

    manager.DestroyEntity(first);
    assert(!manager.HasEntity(first));
    assert(manager.HasEntity(second));

    manager.SetPosZ(second, 33.0f);
    assert(manager.GetPhysicsRevision() > revision);

    const std::uint64_t afterDestroy = manager.GetPhysicsRevision();
    manager.DestroyEntity(first);
    assert(manager.GetPhysicsRevision() == afterDestroy);

    const auto fullChunks = manager.GetChunks<PositionComponent, VelocityComponent, ColliderComponent>();
    bool secondStillPresent = false;
    for (const auto* chunk : fullChunks) {
        for (std::size_t i = 0; i < chunk->count; ++i) {
            if (chunk->entities[i] == second) {
                secondStillPresent = true;
            }
        }
    }
    assert(secondStillPresent);

    std::cout << "BATCH7_ARCHETYPE_MANAGER_SMOKE_OK\n";
    return 0;
}
