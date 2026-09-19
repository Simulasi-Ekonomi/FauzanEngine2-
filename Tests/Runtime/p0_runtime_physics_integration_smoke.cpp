#include "Runtime/NeoRuntime.h"
#include "Runtime/GameplayPhysicsBody.h"

#include <cmath>
#include <cstdio>

int main() {
    using namespace NeoEngine;

    NeoRuntime runtime;
    RuntimeConfig config{};
    config.fixedTicksPerFrame = 1U;

    if (!runtime.Initialize(config)) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: initialize failed\n");
        return 1;
    }
    if (runtime.Physics() == nullptr || runtime.ECS() == nullptr || runtime.Scene() == nullptr) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: canonical ownership missing\n");
        runtime.Shutdown();
        return 2;
    }

    GameplayPhysicsBodyBuilder bodies;
    EntityID body = 0U;
    if (!bodies.CreateCircleBody(*runtime.ECS(),
                                 {GameplayPhysicsBodyType::Dynamic, 0.0F, 0.0F,
                                  1.0F, 0.0F, 0.25F, 1.0F},
                                 body)) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: body creation failed\n");
        runtime.Shutdown();
        return 3;
    }

    float beforeX = 0.0F;
    bool foundBefore = false;
    for (ArchetypeChunk* chunk : runtime.ECS()->GetChunks<PositionComponent, VelocityComponent, ColliderComponent>()) {
        for (size_t i = 0U; i < chunk->count; ++i) {
            if (chunk->entities[i] == body) {
                beforeX = chunk->posX[i];
                foundBefore = true;
            }
        }
    }
    if (!foundBefore) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: body missing before tick\n");
        runtime.Shutdown();
        return 4;
    }

    if (!runtime.Tick()) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: canonical runtime tick failed\n");
        runtime.Shutdown();
        return 5;
    }

    float afterX = beforeX;
    bool foundAfter = false;
    for (ArchetypeChunk* chunk : runtime.ECS()->GetChunks<PositionComponent, VelocityComponent, ColliderComponent>()) {
        for (size_t i = 0U; i < chunk->count; ++i) {
            if (chunk->entities[i] == body) {
                afterX = chunk->posX[i];
                foundAfter = true;
            }
        }
    }

    if (!foundAfter || !std::isfinite(afterX) || !(afterX > beforeX)) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: XPBD did not advance canonical ECS body (%f -> %f)\n",
                     beforeX, afterX);
        runtime.Shutdown();
        return 6;
    }

    if (!runtime.Shutdown()) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: shutdown failed\n");
        return 7;
    }

    std::printf("P0_PHYSICS_SMOKE: PASS before=%f after=%f manifolds=%zu\n",
                beforeX, afterX, runtime.Physics() == nullptr ? 0U : runtime.Physics()->GetManifoldCount());
    return 0;
}
