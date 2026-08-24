#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Threading/JobSystem.h"

#include <cstdio>
#include <vector>

using namespace NeoEngine;

static std::vector<uint8_t> runReplay() {
    ArchetypeManager entities;
    XPBDPhysicsSystem physics;
    const uint32_t flags = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;
    const EntityID a = entities.CreateEntity(flags);
    const EntityID b = entities.CreateEntity(flags);
    const EntityID c = entities.CreateEntity(flags);
    const EntityID d = entities.CreateEntity(flags);
    const EntityID ids[] = {a, b, c, d};
    const float positions[] = {0.0f, 0.45f, 0.90f, 1.35f};

    for (size_t index = 0; index < 4; ++index) {
        entities.SetPosX(ids[index], positions[index]);
        entities.SetRadius(ids[index], 0.35f);
        entities.SetInvMass(ids[index], index == 3 ? 0.0f : 1.0f);
        if (index == 0) entities.SetVelX(ids[index], 0.1f);
    }
    physics.AddDistanceJoint(a, b, 0.2f, 0.8f);
    physics.AddDistanceJoint(b, c, 0.2f, 0.8f);

    for (int frame = 0; frame < 8; ++frame)
        physics.Step(entities, 1.0f / 60.0f);
    return physics.SerializePhysicsState();
}

int main() {
    JobSystem::Get().Initialize(4);
    const auto first = runReplay();
    const auto second = runReplay();
    const bool ok = !first.empty() && first == second;
    if (ok) {
        std::printf("XPBD_DETERMINISM_OK bytes=%zu\n", first.size());
    } else {
        std::fprintf(stderr, "XPBD_DETERMINISM_FAIL first=%zu second=%zu\n", first.size(), second.size());
    }
    JobSystem::Get().Shutdown();
    return ok ? 0 : 1;
}
