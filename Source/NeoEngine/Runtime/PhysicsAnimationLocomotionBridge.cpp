#include "Runtime/PhysicsAnimationLocomotionBridge.h"

namespace NeoEngine {
bool PhysicsAnimationLocomotionBridge::Apply(ArchetypeManager& entities, EntityID body, GameplayPhysicsBodyBuilder& bodies, AnimationLocomotionBridge& locomotion, AnimationStateMachine& machine) {
    GameplayCircleBodySnapshot snapshot{};
    if (!bodies.SnapshotCircleBody(entities, body, snapshot)) { lastError_ = PhysicsAnimationLocomotionBridgeError::SnapshotFailed; return false; }
    if (!locomotion.Apply({snapshot.velocityX, snapshot.velocityZ}, machine)) { lastError_ = PhysicsAnimationLocomotionBridgeError::LocomotionRejected; return false; }
    lastError_ = PhysicsAnimationLocomotionBridgeError::None; return true;
}
} // namespace NeoEngine
