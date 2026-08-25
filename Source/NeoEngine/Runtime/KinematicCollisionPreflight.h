#pragma once

#include "GameplayPhysicsQuery.h"
#include "KinematicMotionController.h"

#include <cstdint>

namespace NeoEngine {
enum class KinematicCollisionPreflightError : uint8_t { None, NotInitialized, InvalidConfiguration, InvalidInput, MissingTransform, QueryFailed, Blocked, MotionFailed };
struct KinematicCollisionPreflightConfig { CollisionMask mask = COLLISION_LAYER_DEFAULT; float clearance = 0.0F; };

// Collision preflight only. It reads XPBD snapshots and delegates any transform mutation
// exclusively to the supplied KinematicMotionController; it never calls XPBD Step.
class KinematicCollisionPreflight {
public:
    bool Initialize(KinematicCollisionPreflightConfig config = {});
    bool Step(const XPBDPhysicsSystem& physics, SceneWorld& world, SceneEntity entity, KinematicPlanarInput input, float seconds, KinematicMotionController& motion);
    [[nodiscard]] KinematicCollisionPreflightError LastError() const { return lastError_; }
    [[nodiscard]] bool IsReady() const { return initialized_; }
private:
    KinematicCollisionPreflightConfig config_{};
    GameplayPhysicsQuery query_{};
    KinematicCollisionPreflightError lastError_ = KinematicCollisionPreflightError::NotInitialized;
    bool initialized_ = false;
};
} // namespace NeoEngine
