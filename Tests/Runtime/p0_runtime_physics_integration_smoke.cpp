#include "Runtime/NeoRuntime.h"

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

    SceneEntity actor{};
    if (!runtime.Scene()->Create(actor) ||
        !runtime.Scene()->SetTransform(actor, {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F})) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: scene actor creation failed\n");
        runtime.Shutdown();
        return 3;
    }

    EntityID physicsBody = 0U;
    const GameplayCircleBodyConfig bodyConfig{
        GameplayPhysicsBodyType::Dynamic, 0.0F, 0.0F, 1.0F, 0.0F, 0.25F, 1.0F
    };
    if (!runtime.CreatePhysicsCircleBody(actor, bodyConfig, physicsBody) || physicsBody == 0U) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: scene physics binding failed\n");
        runtime.Shutdown();
        return 4;
    }
    if (!runtime.PhysicsPoseSync().IsPhysicsAuthoritative(actor)) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: dynamic body is not physics authoritative\n");
        runtime.Shutdown();
        return 5;
    }

    const Transform3* before = runtime.Scene()->GetTransform(actor);
    if (before == nullptr || !std::isfinite(before->x)) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: scene pose missing before tick\n");
        runtime.Shutdown();
        return 6;
    }
    const float beforeX = before->x;

    if (!runtime.Tick()) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: canonical runtime tick failed\n");
        runtime.Shutdown();
        return 7;
    }

    const Transform3* after = runtime.Scene()->GetTransform(actor);
    const NeoRuntimeFrameReceipt* receipt = runtime.LastFrameReceipt();
    if (after == nullptr || receipt == nullptr || !std::isfinite(after->x) ||
        !(after->x > beforeX) || receipt->physicsBodyCount != 1U ||
        receipt->physicsStepMicroseconds == 0U) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: physics did not propagate SceneWorld pose (%f -> %f), bodies=%u step_us=%llu\n",
                     beforeX, after == nullptr ? 0.0F : after->x,
                     receipt == nullptr ? 0U : receipt->physicsBodyCount,
                     static_cast<unsigned long long>(receipt == nullptr ? 0U : receipt->physicsStepMicroseconds));
        runtime.Shutdown();
        return 8;
    }

    const float finalX = after->x;
    const uint32_t finalBodies = receipt->physicsBodyCount;
    const uint64_t finalStepUs = receipt->physicsStepMicroseconds;

    if (!runtime.DestroyPhysicsBody(actor)) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: physics body destruction failed\n");
        runtime.Shutdown();
        return 9;
    }
    if (runtime.PhysicsPoseSync().BindingCount() != 0U) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: physics binding leaked after destroy\n");
        runtime.Shutdown();
        return 10;
    }

    if (!runtime.Shutdown()) {
        std::fprintf(stderr, "P0_PHYSICS_SMOKE: shutdown failed\n");
        return 11;
    }

    std::printf("P0_PHYSICS_SMOKE: PASS before=%f after=%f bodies=%u step_us=%llu\n",
                beforeX, after == nullptr ? 0.0F : after->x,
                receipt == nullptr ? 0U : receipt->physicsBodyCount,
                static_cast<unsigned long long>(receipt == nullptr ? 0U : receipt->physicsStepMicroseconds));
    return 0;
}
