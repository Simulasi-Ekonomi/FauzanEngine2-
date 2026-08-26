#pragma once

#include "AnimationTimeline.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {
enum class AnimationStateMachineError : uint8_t { None, InvalidState, DuplicateState, Capacity, MissingState, InvalidTransition, DuplicateTransition, NotStarted, TransitionInProgress, InvalidDelta, SampleFailed, InvalidSnapshot, EventCollectionFailed };
struct AnimationStateSpec { std::string id; std::string trackId; AnimationPlayback playback = AnimationPlayback::Clamp; };
struct AnimationTransitionSpec { std::string id; std::string fromStateId; std::string toStateId; float durationSeconds = 0.0F; };
struct AnimationStateMachineSnapshot { std::string activeStateId; std::string targetStateId; std::string transitionId; bool blending = false; float blendFraction = 0.0F; float activeTimeSeconds = 0.0F; float targetTimeSeconds = 0.0F; };

// Bounded scalar state/blend selector. It owns neither timelines nor SceneWorld and
// therefore cannot write transforms, movement routes, or movement authority.
class AnimationStateMachine {
public:
    static constexpr uint8_t kMaxStates = 32;
    static constexpr uint8_t kMaxTransitions = 64;
    static constexpr uint16_t kMaxEventsPerCollection = 128U;
    static constexpr uint8_t kMaxIdentifierBytes = 64;
    static constexpr float kMaxDeltaSeconds = 1.0F;
    bool AddState(AnimationStateSpec state);
    bool AddTransition(AnimationTransitionSpec transition);
    bool Start(const std::string& stateId);
    bool Reset();
    bool Trigger(const std::string& transitionId);
    bool Update(float deltaSeconds);
    bool Sample(const AnimationTimeline& timeline, float& value) const;
    bool CollectEvents(const AnimationTimeline& timeline, float fromTime, float toTime, std::vector<std::string>& output) const;
    bool Snapshot(AnimationStateMachineSnapshot& snapshot) const;
    bool Restore(const AnimationStateMachineSnapshot& snapshot);
    [[nodiscard]] std::string ActiveStateId() const;
    [[nodiscard]] bool IsBlending() const { return transitionIndex_ >= 0; }
    [[nodiscard]] AnimationStateMachineError LastError() const { return lastError_; }
private:
    struct State { AnimationStateSpec spec; };
    struct Transition { AnimationTransitionSpec spec; size_t fromIndex = 0; size_t toIndex = 0; };
    [[nodiscard]] int FindState(const std::string& stateId) const;
    std::vector<State> states_;
    std::vector<Transition> transitions_;
    int activeStateIndex_ = -1;
    int transitionIndex_ = -1;
    size_t blendTargetIndex_ = 0;
    float activeTime_ = 0.0F;
    float targetTime_ = 0.0F;
    float blendElapsed_ = 0.0F;
    mutable AnimationStateMachineError lastError_ = AnimationStateMachineError::None;
};
} // namespace NeoEngine
