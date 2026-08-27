#include "Runtime/AnimationLocomotionBridge.h"

#include <cmath>

namespace NeoEngine {
bool AnimationLocomotionBridge::Initialize(AnimationLocomotionBridgeConfig config) {
    const bool hasIdleState = !config.idleStateId.empty();
    const bool hasLocomotionState = !config.locomotionStateId.empty();
    if (config.idleToLocomotionTransitionId.empty() || config.locomotionToIdleTransitionId.empty() || hasIdleState != hasLocomotionState || config.idleToLocomotionTransitionId.find('\0') != std::string::npos || config.locomotionToIdleTransitionId.find('\0') != std::string::npos || config.idleStateId.find('\0') != std::string::npos || config.locomotionStateId.find('\0') != std::string::npos || config.idleToLocomotionTransitionId.size() > 64U || config.locomotionToIdleTransitionId.size() > 64U || config.idleStateId.size() > 64U || config.locomotionStateId.size() > 64U || config.idleToLocomotionTransitionId == config.locomotionToIdleTransitionId || (hasIdleState && config.idleStateId == config.locomotionStateId) || !std::isfinite(config.movementThreshold) || config.movementThreshold < 0.0F) { lastError_ = AnimationLocomotionBridgeError::InvalidConfiguration; return false; }
    config_ = std::move(config); initialized_ = true; locomoting_ = false; lastError_ = AnimationLocomotionBridgeError::None; return true;
}
bool AnimationLocomotionBridge::Apply(KinematicPlanarInput input, AnimationStateMachine& machine) {
    if (!initialized_) { lastError_ = AnimationLocomotionBridgeError::InvalidConfiguration; return false; }
    if (!std::isfinite(input.x) || !std::isfinite(input.z)) { lastError_ = AnimationLocomotionBridgeError::InvalidInput; return false; }
    if (config_.idleStateId.empty()) {
        if (machine.ActiveStateId().empty()) { lastError_ = AnimationLocomotionBridgeError::StateMachineNotStarted; return false; }
    } else {
        AnimationStateMachineSnapshot machineSnapshot{};
        if (!machine.Snapshot(machineSnapshot)) { lastError_ = machine.LastError() == AnimationStateMachineError::NotStarted ? AnimationLocomotionBridgeError::StateMachineNotStarted : AnimationLocomotionBridgeError::StateTriggerFailed; return false; }
        const std::string& observedState = machineSnapshot.blending ? machineSnapshot.targetStateId : machineSnapshot.activeStateId;
        if (observedState != config_.idleStateId && observedState != config_.locomotionStateId) { lastError_ = AnimationLocomotionBridgeError::StateMismatch; return false; }
        locomoting_ = observedState == config_.locomotionStateId;
    }
    const double inputMagnitudeSquared = (static_cast<double>(input.x) * static_cast<double>(input.x)) + (static_cast<double>(input.z) * static_cast<double>(input.z));
    const double thresholdSquared = static_cast<double>(config_.movementThreshold) * static_cast<double>(config_.movementThreshold);
    const bool desiredLocomotion = inputMagnitudeSquared > thresholdSquared;
    if (desiredLocomotion == locomoting_) { lastError_ = AnimationLocomotionBridgeError::None; return true; }
    if (!machine.Trigger(desiredLocomotion ? config_.idleToLocomotionTransitionId : config_.locomotionToIdleTransitionId)) { lastError_ = AnimationLocomotionBridgeError::StateTriggerFailed; return false; }
    locomoting_ = desiredLocomotion; lastError_ = AnimationLocomotionBridgeError::None; return true;
}
} // namespace NeoEngine
