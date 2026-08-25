#include "Runtime/AnimationLocomotionBridge.h"

#include <cmath>

namespace NeoEngine {
bool AnimationLocomotionBridge::Initialize(AnimationLocomotionBridgeConfig config) {
    if (config.idleToLocomotionTransitionId.empty() || config.locomotionToIdleTransitionId.empty() || !std::isfinite(config.movementThreshold) || config.movementThreshold < 0.0F) { lastError_ = AnimationLocomotionBridgeError::InvalidConfiguration; return false; }
    config_ = std::move(config); initialized_ = true; locomoting_ = false; lastError_ = AnimationLocomotionBridgeError::None; return true;
}
bool AnimationLocomotionBridge::Apply(KinematicPlanarInput input, AnimationStateMachine& machine) {
    if (!initialized_) { lastError_ = AnimationLocomotionBridgeError::InvalidConfiguration; return false; }
    if (!std::isfinite(input.x) || !std::isfinite(input.z)) { lastError_ = AnimationLocomotionBridgeError::InvalidInput; return false; }
    if (machine.ActiveStateId().empty()) { lastError_ = AnimationLocomotionBridgeError::StateMachineNotStarted; return false; }
    const bool desiredLocomotion = (input.x * input.x) + (input.z * input.z) > config_.movementThreshold * config_.movementThreshold;
    if (desiredLocomotion == locomoting_) { lastError_ = AnimationLocomotionBridgeError::None; return true; }
    if (!machine.Trigger(desiredLocomotion ? config_.idleToLocomotionTransitionId : config_.locomotionToIdleTransitionId)) { lastError_ = AnimationLocomotionBridgeError::StateTriggerFailed; return false; }
    locomoting_ = desiredLocomotion; lastError_ = AnimationLocomotionBridgeError::None; return true;
}
} // namespace NeoEngine
