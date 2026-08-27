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
    const double inputMagnitudeSquared = (static_cast<double>(input.x) * static_cast<double>(input.x)) + (static_cast<double>(input.z) * static_cast<double>(input.z));
    const double thresholdSquared = static_cast<double>(config_.movementThreshold) * static_cast<double>(config_.movementThreshold);
    const bool desiredLocomotion = inputMagnitudeSquared > thresholdSquared;
    if (desiredLocomotion == locomoting_) { lastError_ = AnimationLocomotionBridgeError::None; return true; }
    if (!machine.Trigger(desiredLocomotion ? config_.idleToLocomotionTransitionId : config_.locomotionToIdleTransitionId)) { lastError_ = AnimationLocomotionBridgeError::StateTriggerFailed; return false; }
    locomoting_ = desiredLocomotion; lastError_ = AnimationLocomotionBridgeError::None; return true;
}
} // namespace NeoEngine
