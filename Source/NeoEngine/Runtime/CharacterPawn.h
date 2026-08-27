#pragma once

#include "ActorComponentWorld.h"
#include "AnimationStateMachine.h"
#include "AssetResourceManager.h"
#include "MovementAuthority.h"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace NeoEngine {

enum class CharacterPawnError : uint8_t {
    None,
    NotInitialized,
    InvalidConfig,
    InvalidInput,
    InvalidTickCount,
    InvalidRootMotion,
    AuthorityRejected,
    SceneApplyRejected,
    AnimationRejected,
    AlreadyAttached,
};

enum class CharacterRootMotionMode : uint8_t { Kinematic, SkeletalRoot };

enum class CharacterMovementAuthority : uint8_t { None, KinematicRoute, SkeletalRoot };

struct CharacterPawnConfig {
    float fixedSeconds = 1.0F / 60.0F;
    float walkSpeed = 2.0F;
    float runSpeed = 5.0F;
    float jumpVelocity = 5.0F;
    float gravity = 9.8F;
    float maxPlanarInput = 1.0F;
};

struct CharacterPawnInput {
    float moveX = 0.0F;
    float moveZ = 0.0F;
    bool sprint = false;
    bool jump = false;
};

struct CharacterRootMotionDelta {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

struct CharacterAnimationGraphSnapshot {
    AnimationStateMachineSnapshot base{};
    AnimationStateMachineSnapshot overlay{};
    uint16_t overlayWeightPermille = 1000U;
    bool hasOverlay = false;
};

struct CharacterPawnSnapshot {
    SceneEntity actor{};
    CharacterMovementAuthority authority = CharacterMovementAuthority::None;
    CharacterRootMotionMode rootMotionMode = CharacterRootMotionMode::Kinematic;
    CharacterRootMotionDelta velocity{};
    bool grounded = true;
    CharacterPawnInput pendingInput{};
    CharacterRootMotionDelta pendingRootMotion{};
    CharacterAnimationGraphSnapshot animation{};
};

class CharacterAnimationGraph {
public:
    static constexpr uint8_t kMaxLayers = 2U;
    static constexpr uint16_t kMaxEventsPerCollection = 256U;

    bool AddBaseState(AnimationStateSpec state);
    bool AddBaseTransition(AnimationTransitionSpec transition);
    bool StartBase(std::string_view stateId);
    bool TriggerBase(std::string_view transitionId);
    bool AddOverlayState(AnimationStateSpec state);
    bool AddOverlayTransition(AnimationTransitionSpec transition);
    bool StartOverlay(std::string_view stateId);
    bool TriggerOverlay(std::string_view transitionId);
    bool Tick(float deltaSeconds);
    bool SetOverlayWeightPermille(uint16_t weightPermille);
    bool Sample(const AnimationTimeline& timeline, float& value) const;
    bool CollectAnimationEvents(const AnimationTimeline& timeline, float fromTime, float toTime, std::vector<std::string>& output) const;
    bool Snapshot(CharacterAnimationGraphSnapshot& snapshot) const;
    bool Restore(const CharacterAnimationGraphSnapshot& snapshot);
    [[nodiscard]] bool HasBase() const { return hasBase_; }
    [[nodiscard]] bool IsBaseBlending() const { return base_.IsBlending(); }
    [[nodiscard]] bool HasOverlay() const { return hasOverlay_; }
    [[nodiscard]] const std::string& ActiveBaseState() const { return baseState_; }
    [[nodiscard]] const std::string& ActiveOverlayState() const { return overlayState_; }
    [[nodiscard]] AnimationStateMachineError LastError() const { return lastError_; }

private:
    AnimationStateMachine base_{};
    AnimationStateMachine overlay_{};
    std::string baseState_{};
    std::string overlayState_{};
    bool hasBase_ = false;
    bool hasOverlay_ = false;
    bool baseStarted_ = false;
    bool overlayStarted_ = false;
    uint16_t overlayWeightPermille_ = 1000U;
    mutable AnimationStateMachineError lastError_ = AnimationStateMachineError::None;
};

struct CharacterTransitionBinding {
    std::string from;
    std::string to;
    std::string transitionId;
};

class CharacterPawn final : public IActorComponent {
public:
    static constexpr uint16_t kTypeId = 100U;
    static constexpr uint16_t kComponentSnapshotBytes = 512U;
    static constexpr uint8_t kMaxTransitionBindings = 16U;

    explicit CharacterPawn(CharacterPawnConfig config = {});
    [[nodiscard]] uint16_t TypeId() const override { return kTypeId; }
    [[nodiscard]] std::string_view TypeName() const override { return "CharacterPawn"; }
    [[nodiscard]] bool OnAttach(SceneWorld& world, SceneEntity actor) override;
    [[nodiscard]] bool OnDetach(SceneWorld& world, SceneEntity actor) override;
    [[nodiscard]] bool OnFixedTick(SceneWorld& world, SceneEntity actor, uint32_t fixedTicks) override;
    [[nodiscard]] uint16_t SnapshotSizeBytes() const override { return kComponentSnapshotBytes; }
    [[nodiscard]] bool CaptureSnapshot(std::span<uint8_t> bytes) const override;
    [[nodiscard]] bool ValidateSnapshot(std::span<const uint8_t> bytes) const override;
    [[nodiscard]] bool RestoreSnapshot(std::span<const uint8_t> bytes) override;
    [[nodiscard]] bool CollectAnimationEvents(const AnimationTimeline& timeline, float fromTime, float toTime, std::vector<std::string>& output) const;

    [[nodiscard]] bool BindMovementAuthorityGate(MovementAuthorityGate* gate);
    [[nodiscard]] bool BindAnimationResource(AssetResourceManager* resources, AssetResourceHandle handle);
    [[nodiscard]] bool SubmitInput(const CharacterPawnInput& input);
    [[nodiscard]] bool SubmitRootMotion(const CharacterRootMotionDelta& delta);
    [[nodiscard]] bool SetRootMotionMode(CharacterRootMotionMode mode);
    [[nodiscard]] bool SetTransitionBinding(CharacterTransitionBinding binding);
    [[nodiscard]] bool TriggerOverlay(std::string_view transitionId);
    [[nodiscard]] bool Snapshot(CharacterPawnSnapshot& snapshot) const;
    [[nodiscard]] bool Restore(const CharacterPawnSnapshot& snapshot);
    [[nodiscard]] CharacterAnimationGraph& AnimationGraph() { return animation_; }
    [[nodiscard]] const CharacterAnimationGraph& AnimationGraph() const { return animation_; }
    [[nodiscard]] bool IsAttached() const { return attached_; }
    [[nodiscard]] CharacterPawnError LastError() const { return lastError_; }

private:
    bool Fail(CharacterPawnError error);
    bool ValidateInput(const CharacterPawnInput& input) const;
    bool ApplyOneFixedStep(SceneWorld& world, const CharacterPawnInput& input, const CharacterRootMotionDelta& rootMotion);
    bool SelectLocomotionState(CharacterAnimationGraph& animation, const CharacterPawnInput& input) const;
    const std::string* FindTransition(std::string_view from, std::string_view to) const;

    CharacterPawnConfig config_{};
    CharacterPawnInput pendingInput_{};
    CharacterRootMotionDelta pendingRootMotion_{};
    CharacterAnimationGraph animation_{};
    std::array<CharacterTransitionBinding, kMaxTransitionBindings> transitionBindings_{};
    uint8_t transitionBindingCount_ = 0U;
    SceneEntity actor_{};
    CharacterRootMotionMode rootMotionMode_ = CharacterRootMotionMode::Kinematic;
    CharacterMovementAuthority lastAuthority_ = CharacterMovementAuthority::None;
    CharacterRootMotionDelta velocity_{};
    MovementAuthorityGate ownedAuthorityGate_{};
    MovementAuthorityGate* authorityGate_ = &ownedAuthorityGate_;
    AssetResourceManager* animationResources_ = nullptr;
    AssetResourceHandle animationResource_{};
    bool grounded_ = true;
    bool attached_ = false;
    mutable CharacterPawnError lastError_ = CharacterPawnError::NotInitialized;
};

} // namespace NeoEngine
