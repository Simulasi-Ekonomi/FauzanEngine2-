#pragma once

#include "AnimationLocomotionBridge.h"
#include "GameplayPhysicsBody.h"

#include <cstdint>

namespace NeoEngine {
enum class PhysicsAnimationLocomotionBridgeError : uint8_t { None, SnapshotFailed, LocomotionRejected };

// Reads canonical ECS velocity, then delegates only scalar locomotion selection.
// It does not step XPBD or own SceneWorld/RouteIntent/movement transform authority.
class PhysicsAnimationLocomotionBridge {
public:
    bool Apply(ArchetypeManager& entities, EntityID body, GameplayPhysicsBodyBuilder& bodies, AnimationLocomotionBridge& locomotion, AnimationStateMachine& machine);
    [[nodiscard]] PhysicsAnimationLocomotionBridgeError LastError() const { return lastError_; }
private:
    PhysicsAnimationLocomotionBridgeError lastError_ = PhysicsAnimationLocomotionBridgeError::None;
};
} // namespace NeoEngine
