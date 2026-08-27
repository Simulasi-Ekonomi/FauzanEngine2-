#pragma once

#include "AnimationStateMachine.h"
#include "KinematicMotionController.h"

#include <cstdint>
#include <string>

namespace NeoEngine {
enum class AnimationLocomotionBridgeError : uint8_t { None, InvalidConfiguration, StateMachineNotStarted, InvalidInput, StateTriggerFailed, StateMismatch };
struct AnimationLocomotionBridgeConfig { std::string idleToLocomotionTransitionId; std::string locomotionToIdleTransitionId; float movementThreshold = 0.01F; std::string idleStateId; std::string locomotionStateId; };

// Input-to-animation selector only: no SceneWorld, RouteIntent, movement authority,
// transform, controller stepping, or root-motion ownership is present in this type.
class AnimationLocomotionBridge {
public:
    bool Initialize(AnimationLocomotionBridgeConfig config);
    bool Apply(KinematicPlanarInput input, AnimationStateMachine& machine);
    [[nodiscard]] bool IsReady() const { return initialized_; }
    [[nodiscard]] bool IsLocomoting() const { return locomoting_; }
    [[nodiscard]] AnimationLocomotionBridgeError LastError() const { return lastError_; }
private:
    AnimationLocomotionBridgeConfig config_{};
    bool initialized_ = false;
    bool locomoting_ = false;
    AnimationLocomotionBridgeError lastError_ = AnimationLocomotionBridgeError::InvalidConfiguration;
};
} // namespace NeoEngine
