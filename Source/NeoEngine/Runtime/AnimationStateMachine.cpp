#include "Runtime/AnimationStateMachine.h"

#include <algorithm>
#include <cmath>

namespace NeoEngine {
int AnimationStateMachine::FindState(const std::string& stateId) const { const auto found = std::find_if(states_.begin(), states_.end(), [&stateId](const State& state) { return state.spec.id == stateId; }); return found == states_.end() ? -1 : static_cast<int>(found - states_.begin()); }
bool AnimationStateMachine::AddState(AnimationStateSpec state) {
    if (state.id.empty() || state.trackId.empty()) { lastError_ = AnimationStateMachineError::InvalidState; return false; }
    if (FindState(state.id) >= 0) { lastError_ = AnimationStateMachineError::DuplicateState; return false; }
    if (states_.size() >= kMaxStates) { lastError_ = AnimationStateMachineError::Capacity; return false; }
    states_.push_back({std::move(state)}); lastError_ = AnimationStateMachineError::None; return true;
}
bool AnimationStateMachine::AddTransition(AnimationTransitionSpec transition) {
    if (transition.id.empty() || !std::isfinite(transition.durationSeconds) || transition.durationSeconds < 0.0F) { lastError_ = AnimationStateMachineError::InvalidTransition; return false; }
    if (std::any_of(transitions_.begin(), transitions_.end(), [&transition](const Transition& candidate) { return candidate.spec.id == transition.id; })) { lastError_ = AnimationStateMachineError::DuplicateTransition; return false; }
    const int from = FindState(transition.fromStateId); const int to = FindState(transition.toStateId);
    if (from < 0 || to < 0 || from == to) { lastError_ = AnimationStateMachineError::InvalidTransition; return false; }
    if (transitions_.size() >= kMaxTransitions) { lastError_ = AnimationStateMachineError::Capacity; return false; }
    transitions_.push_back({std::move(transition), static_cast<size_t>(from), static_cast<size_t>(to)}); lastError_ = AnimationStateMachineError::None; return true;
}
bool AnimationStateMachine::Start(const std::string& stateId) { const int index = FindState(stateId); if (index < 0) { lastError_ = AnimationStateMachineError::MissingState; return false; } activeStateIndex_ = index; transitionIndex_ = -1; activeTime_ = 0.0F; targetTime_ = 0.0F; blendElapsed_ = 0.0F; lastError_ = AnimationStateMachineError::None; return true; }
bool AnimationStateMachine::Trigger(const std::string& transitionId) {
    if (activeStateIndex_ < 0) { lastError_ = AnimationStateMachineError::NotStarted; return false; }
    if (transitionIndex_ >= 0) { lastError_ = AnimationStateMachineError::TransitionInProgress; return false; }
    const auto found = std::find_if(transitions_.begin(), transitions_.end(), [&transitionId](const Transition& transition) { return transition.spec.id == transitionId; });
    if (found == transitions_.end() || found->fromIndex != static_cast<size_t>(activeStateIndex_)) { lastError_ = AnimationStateMachineError::InvalidTransition; return false; }
    if (found->spec.durationSeconds == 0.0F) { activeStateIndex_ = static_cast<int>(found->toIndex); activeTime_ = 0.0F; lastError_ = AnimationStateMachineError::None; return true; }
    transitionIndex_ = static_cast<int>(found - transitions_.begin()); blendTargetIndex_ = found->toIndex; targetTime_ = 0.0F; blendElapsed_ = 0.0F; lastError_ = AnimationStateMachineError::None; return true;
}
bool AnimationStateMachine::Update(float deltaSeconds) {
    if (activeStateIndex_ < 0) { lastError_ = AnimationStateMachineError::NotStarted; return false; }
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0F || !std::isfinite(activeTime_ + deltaSeconds) || (transitionIndex_ >= 0 && (!std::isfinite(targetTime_ + deltaSeconds) || !std::isfinite(blendElapsed_ + deltaSeconds)))) { lastError_ = AnimationStateMachineError::InvalidDelta; return false; }
    activeTime_ += deltaSeconds;
    if (transitionIndex_ >= 0) { targetTime_ += deltaSeconds; blendElapsed_ += deltaSeconds; const Transition& transition = transitions_[static_cast<size_t>(transitionIndex_)]; if (blendElapsed_ >= transition.spec.durationSeconds) { activeStateIndex_ = static_cast<int>(blendTargetIndex_); activeTime_ = targetTime_; transitionIndex_ = -1; blendElapsed_ = 0.0F; } }
    lastError_ = AnimationStateMachineError::None; return true;
}
bool AnimationStateMachine::Sample(const AnimationTimeline& timeline, float& value) const {
    if (activeStateIndex_ < 0) { lastError_ = AnimationStateMachineError::NotStarted; return false; }
    const State& source = states_[static_cast<size_t>(activeStateIndex_)]; float sourceValue = 0.0F;
    if (!timeline.Sample(source.spec.trackId, activeTime_, source.spec.playback, sourceValue)) { lastError_ = AnimationStateMachineError::SampleFailed; return false; }
    if (transitionIndex_ < 0) { value = sourceValue; lastError_ = AnimationStateMachineError::None; return true; }
    const Transition& transition = transitions_[static_cast<size_t>(transitionIndex_)]; const State& target = states_[blendTargetIndex_]; float targetValue = 0.0F;
    if (!timeline.Sample(target.spec.trackId, targetTime_, target.spec.playback, targetValue)) { lastError_ = AnimationStateMachineError::SampleFailed; return false; }
    value = sourceValue + ((targetValue - sourceValue) * std::clamp(blendElapsed_ / transition.spec.durationSeconds, 0.0F, 1.0F)); lastError_ = AnimationStateMachineError::None; return true;
}
bool AnimationStateMachine::Snapshot(AnimationStateMachineSnapshot& snapshot) const {
    if (activeStateIndex_ < 0) { lastError_ = AnimationStateMachineError::NotStarted; return false; }
    AnimationStateMachineSnapshot candidate{};
    candidate.activeStateId = states_[static_cast<size_t>(activeStateIndex_)].spec.id;
    candidate.activeTimeSeconds = activeTime_;
    candidate.targetTimeSeconds = targetTime_;
    candidate.blending = transitionIndex_ >= 0;
    if (candidate.blending) {
        const Transition& transition = transitions_[static_cast<size_t>(transitionIndex_)];
        candidate.targetStateId = states_[blendTargetIndex_].spec.id;
        candidate.blendFraction = std::clamp(blendElapsed_ / transition.spec.durationSeconds, 0.0F, 1.0F);
    }
    snapshot = std::move(candidate); lastError_ = AnimationStateMachineError::None; return true;
}
std::string AnimationStateMachine::ActiveStateId() const { return activeStateIndex_ < 0 ? std::string{} : states_[static_cast<size_t>(activeStateIndex_)].spec.id; }
} // namespace NeoEngine
