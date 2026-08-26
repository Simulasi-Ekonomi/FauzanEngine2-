#include "CharacterPawn.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace NeoEngine {
namespace {
constexpr float kMaxRootMotionMagnitude = 100.0F;
constexpr float kMaxFixedSeconds = 1.0F;
bool Finite(float value) { return std::isfinite(value); }
bool ValidConfig(const CharacterPawnConfig& config) {
    return Finite(config.fixedSeconds) && config.fixedSeconds > 0.0F && config.fixedSeconds <= kMaxFixedSeconds && Finite(config.walkSpeed) && config.walkSpeed >= 0.0F && Finite(config.runSpeed) && config.runSpeed >= config.walkSpeed && Finite(config.jumpVelocity) && config.jumpVelocity >= 0.0F && Finite(config.gravity) && config.gravity >= 0.0F && Finite(config.maxPlanarInput) && config.maxPlanarInput > 0.0F && config.maxPlanarInput <= 1.0F;
}
bool ValidRootMotion(const CharacterRootMotionDelta& delta) {
    return Finite(delta.x) && Finite(delta.y) && Finite(delta.z) && std::abs(delta.x) <= kMaxRootMotionMagnitude && std::abs(delta.y) <= kMaxRootMotionMagnitude && std::abs(delta.z) <= kMaxRootMotionMagnitude;
}
}

bool CharacterAnimationGraph::AddBaseState(AnimationStateSpec state) {
    if (!base_.AddState(std::move(state))) { lastError_ = base_.LastError(); return false; }
    hasBase_ = true;
    lastError_ = AnimationStateMachineError::None;
    return true;
}

bool CharacterAnimationGraph::AddBaseTransition(AnimationTransitionSpec transition) {
    if (!base_.AddTransition(std::move(transition))) { lastError_ = base_.LastError(); return false; }
    lastError_ = AnimationStateMachineError::None;
    return true;
}

bool CharacterAnimationGraph::StartBase(std::string_view stateId) {
    if (!base_.Start(std::string(stateId))) { lastError_ = base_.LastError(); return false; }
    baseState_ = std::string(stateId);
    baseStarted_ = true;
    lastError_ = AnimationStateMachineError::None;
    return true;
}

bool CharacterAnimationGraph::TriggerBase(std::string_view transitionId) {
    if (!base_.Trigger(std::string(transitionId))) { lastError_ = base_.LastError(); return false; }
    lastError_ = AnimationStateMachineError::None;
    return true;
}

bool CharacterAnimationGraph::AddOverlayState(AnimationStateSpec state) {
    if (!overlay_.AddState(std::move(state))) { lastError_ = overlay_.LastError(); return false; }
    hasOverlay_ = true;
    lastError_ = AnimationStateMachineError::None;
    return true;
}

bool CharacterAnimationGraph::AddOverlayTransition(AnimationTransitionSpec transition) {
    if (!overlay_.AddTransition(std::move(transition))) { lastError_ = overlay_.LastError(); return false; }
    lastError_ = AnimationStateMachineError::None;
    return true;
}

bool CharacterAnimationGraph::StartOverlay(std::string_view stateId) {
    if (!overlay_.Start(std::string(stateId))) { lastError_ = overlay_.LastError(); return false; }
    overlayState_ = std::string(stateId);
    overlayStarted_ = true;
    lastError_ = AnimationStateMachineError::None;
    return true;
}

bool CharacterAnimationGraph::TriggerOverlay(std::string_view transitionId) {
    if (!overlay_.Trigger(std::string(transitionId))) { lastError_ = overlay_.LastError(); return false; }
    lastError_ = AnimationStateMachineError::None;
    return true;
}

bool CharacterAnimationGraph::Tick(float deltaSeconds) {
    if (!Finite(deltaSeconds) || deltaSeconds <= 0.0F || deltaSeconds > kMaxFixedSeconds) { lastError_ = AnimationStateMachineError::InvalidDelta; return false; }
    if (baseStarted_ && !base_.Update(deltaSeconds)) { lastError_ = base_.LastError(); return false; }
    if (overlayStarted_ && !overlay_.Update(deltaSeconds)) { lastError_ = overlay_.LastError(); return false; }
    if (baseStarted_) baseState_ = base_.ActiveStateId();
    if (overlayStarted_) overlayState_ = overlay_.ActiveStateId();
    lastError_ = AnimationStateMachineError::None;
    return true;
}

bool CharacterAnimationGraph::Snapshot(CharacterAnimationGraphSnapshot& snapshot) const {
    CharacterAnimationGraphSnapshot candidate{};
    if (baseStarted_ && !base_.Snapshot(candidate.base)) { lastError_ = base_.LastError(); return false; }
    candidate.hasOverlay = hasOverlay_ && overlayStarted_;
    if (candidate.hasOverlay && !overlay_.Snapshot(candidate.overlay)) { lastError_ = overlay_.LastError(); return false; }
    snapshot = std::move(candidate);
    lastError_ = AnimationStateMachineError::None;
    return true;
}

CharacterPawn::CharacterPawn(CharacterPawnConfig config) : config_(config) {}

bool CharacterPawn::Fail(CharacterPawnError error) { lastError_ = error; return false; }

bool CharacterPawn::ValidateInput(const CharacterPawnInput& input) const {
    if (!Finite(input.moveX) || !Finite(input.moveZ)) return false;
    const float magnitudeSquared = input.moveX * input.moveX + input.moveZ * input.moveZ;
    return Finite(magnitudeSquared) && magnitudeSquared <= config_.maxPlanarInput * config_.maxPlanarInput;
}

bool CharacterPawn::OnAttach(SceneWorld& world, SceneEntity actor) {
    if (!ValidConfig(config_) || world.GetTransform(actor) == nullptr) return Fail(CharacterPawnError::InvalidConfig);
    actor_ = actor;
    pendingInput_ = {};
    pendingRootMotion_ = {};
    velocity_ = {};
    grounded_ = true;
    lastAuthority_ = CharacterMovementAuthority::None;
    attached_ = true;
    lastError_ = CharacterPawnError::None;
    return true;
}

bool CharacterPawn::OnDetach(SceneWorld&, SceneEntity actor) {
    if (!attached_ || actor_ != actor) return Fail(CharacterPawnError::NotInitialized);
    attached_ = false;
    lastAuthority_ = CharacterMovementAuthority::None;
    pendingInput_ = {};
    pendingRootMotion_ = {};
    animationResources_ = nullptr;
    animationResource_ = {};
    lastError_ = CharacterPawnError::None;
    return true;
}

bool CharacterPawn::BindMovementAuthorityGate(MovementAuthorityGate* gate) {
    if (attached_ || gate == nullptr) return Fail(CharacterPawnError::AuthorityRejected);
    authorityGate_ = gate;
    lastError_ = CharacterPawnError::None;
    return true;
}

bool CharacterPawn::BindAnimationResource(AssetResourceManager* resources, AssetResourceHandle handle) {
    if (attached_ || resources == nullptr || resources->Data(handle) == nullptr) return Fail(CharacterPawnError::AnimationRejected);
    animationResources_ = resources;
    animationResource_ = handle;
    lastError_ = CharacterPawnError::None;
    return true;
}

bool CharacterPawn::SubmitInput(const CharacterPawnInput& input) {
    if (!attached_) return Fail(CharacterPawnError::NotInitialized);
    if (!ValidateInput(input)) return Fail(CharacterPawnError::InvalidInput);
    pendingInput_ = input;
    lastError_ = CharacterPawnError::None;
    return true;
}

bool CharacterPawn::SubmitRootMotion(const CharacterRootMotionDelta& delta) {
    if (!attached_) return Fail(CharacterPawnError::NotInitialized);
    if (!ValidRootMotion(delta)) return Fail(CharacterPawnError::InvalidRootMotion);
    pendingRootMotion_ = delta;
    lastError_ = CharacterPawnError::None;
    return true;
}

bool CharacterPawn::SetRootMotionMode(CharacterRootMotionMode mode) {
    if (!attached_) return Fail(CharacterPawnError::NotInitialized);
    if (mode != CharacterRootMotionMode::Kinematic && mode != CharacterRootMotionMode::SkeletalRoot) return Fail(CharacterPawnError::InvalidRootMotion);
    rootMotionMode_ = mode;
    lastError_ = CharacterPawnError::None;
    return true;
}

bool CharacterPawn::SetTransitionBinding(CharacterTransitionBinding binding) {
    if (!attached_) return Fail(CharacterPawnError::NotInitialized);
    if (binding.from.empty() || binding.to.empty() || binding.transitionId.empty()) return Fail(CharacterPawnError::AnimationRejected);
    for (uint8_t index = 0U; index < transitionBindingCount_; ++index) {
        if (transitionBindings_[index].from == binding.from && transitionBindings_[index].to == binding.to) {
            transitionBindings_[index] = std::move(binding);
            lastError_ = CharacterPawnError::None;
            return true;
        }
    }
    if (transitionBindingCount_ >= kMaxTransitionBindings) return Fail(CharacterPawnError::AnimationRejected);
    transitionBindings_[transitionBindingCount_++] = std::move(binding);
    lastError_ = CharacterPawnError::None;
    return true;
}

const std::string* CharacterPawn::FindTransition(std::string_view from, std::string_view to) const {
    for (uint8_t index = 0U; index < transitionBindingCount_; ++index) if (transitionBindings_[index].from == from && transitionBindings_[index].to == to) return &transitionBindings_[index].transitionId;
    return nullptr;
}

bool CharacterPawn::SelectLocomotionState(const CharacterPawnInput& input) {
    if (!animation_.HasBase() || animation_.ActiveBaseState().empty() || animation_.IsBaseBlending()) return true;
    const bool moving = std::abs(input.moveX) > 0.0001F || std::abs(input.moveZ) > 0.0001F;
    const std::string target = moving ? (input.sprint ? "run" : "walk") : "idle";
    if (animation_.ActiveBaseState() == target) return true;
    const std::string* transition = FindTransition(animation_.ActiveBaseState(), target);
    if (transition == nullptr) return true;
    if (!animation_.TriggerBase(*transition)) return Fail(CharacterPawnError::AnimationRejected);
    return true;
}

bool CharacterPawn::ApplyOneFixedStep(SceneWorld& world, const CharacterPawnInput& input, const CharacterRootMotionDelta& rootMotion) {
    const Transform3* local = world.GetLocalTransform(actor_);
    if (local == nullptr) return Fail(CharacterPawnError::SceneApplyRejected);
    Transform3 candidate = *local;
    CharacterRootMotionDelta nextVelocity = velocity_;
    bool nextGrounded = grounded_;
    CharacterMovementAuthority nextAuthority = lastAuthority_;
    MovementAuthority requestedAuthority = MovementAuthority::KinematicRoute;
    if (rootMotionMode_ == CharacterRootMotionMode::Kinematic) {
        if (!ValidRootMotion(rootMotion) || std::abs(rootMotion.x) > 0.0001F || std::abs(rootMotion.y) > 0.0001F || std::abs(rootMotion.z) > 0.0001F) return Fail(CharacterPawnError::InvalidRootMotion);
        const float speed = input.sprint ? config_.runSpeed : config_.walkSpeed;
        nextVelocity = {input.moveX * speed, velocity_.y, input.moveZ * speed};
        if (nextGrounded && input.jump) {
            nextVelocity.y = config_.jumpVelocity;
            nextGrounded = false;
        }
        if (!nextGrounded) nextVelocity.y -= config_.gravity * config_.fixedSeconds;
        candidate.x += nextVelocity.x * config_.fixedSeconds;
        candidate.y += nextVelocity.y * config_.fixedSeconds;
        candidate.z += nextVelocity.z * config_.fixedSeconds;
        if (candidate.y <= 0.0F) { candidate.y = 0.0F; nextVelocity.y = 0.0F; nextGrounded = true; }
        nextAuthority = CharacterMovementAuthority::KinematicRoute;
    } else if (rootMotionMode_ == CharacterRootMotionMode::SkeletalRoot) {
        if (!ValidRootMotion(rootMotion)) return Fail(CharacterPawnError::InvalidRootMotion);
        requestedAuthority = MovementAuthority::SkeletalRoot;
        candidate.x += rootMotion.x;
        candidate.y += rootMotion.y;
        candidate.z += rootMotion.z;
        if (candidate.y < 0.0F) candidate.y = 0.0F;
        nextVelocity = rootMotion;
        nextGrounded = candidate.y == 0.0F;
        nextAuthority = CharacterMovementAuthority::SkeletalRoot;
    } else {
        return Fail(CharacterPawnError::InvalidRootMotion);
    }
    if (!Finite(candidate.x) || !Finite(candidate.y) || !Finite(candidate.z) || !Finite(nextVelocity.x) || !Finite(nextVelocity.y) || !Finite(nextVelocity.z)) return Fail(CharacterPawnError::SceneApplyRejected);
    if (authorityGate_ == nullptr || !authorityGate_->Acquire(actor_, requestedAuthority)) return Fail(CharacterPawnError::AuthorityRejected);
    if (!world.SetTransform(actor_, candidate)) return Fail(CharacterPawnError::SceneApplyRejected);
    velocity_ = nextVelocity;
    grounded_ = nextGrounded;
    lastAuthority_ = nextAuthority;
    return true;
}

bool CharacterPawn::OnFixedTick(SceneWorld& world, SceneEntity actor, uint32_t fixedTicks) {
    if (!attached_ || actor_ != actor || fixedTicks == 0U) return Fail(CharacterPawnError::NotInitialized);
    if (animationResources_ != nullptr && animationResources_->Data(animationResource_) == nullptr) return Fail(CharacterPawnError::AnimationRejected);
    if (authorityGate_ == &ownedAuthorityGate_) authorityGate_->BeginFrame();
    for (uint32_t tick = 0U; tick < fixedTicks; ++tick) {
        if (!SelectLocomotionState(pendingInput_) || !animation_.Tick(config_.fixedSeconds) || !ApplyOneFixedStep(world, pendingInput_, pendingRootMotion_)) return lastError_ == CharacterPawnError::None ? Fail(CharacterPawnError::AnimationRejected) : false;
    }
    pendingInput_.jump = false;
    pendingRootMotion_ = {};
    lastError_ = CharacterPawnError::None;
    return true;
}

bool CharacterPawn::TriggerOverlay(std::string_view transitionId) {
    if (!attached_) return Fail(CharacterPawnError::NotInitialized);
    if (!animation_.TriggerOverlay(transitionId)) return Fail(CharacterPawnError::AnimationRejected);
    lastError_ = CharacterPawnError::None;
    return true;
}

bool CharacterAnimationGraph::Restore(const CharacterAnimationGraphSnapshot& snapshot) {
    CharacterAnimationGraph candidate = *this;
    if (snapshot.base.activeStateId.empty()) {
        if (!candidate.base_.Reset()) return false;
        candidate.baseStarted_ = false;
        candidate.baseState_.clear();
    } else {
        if (!candidate.base_.Restore(snapshot.base)) { lastError_ = candidate.base_.LastError(); return false; }
        candidate.baseStarted_ = true;
        candidate.baseState_ = candidate.base_.ActiveStateId();
    }
    if (snapshot.hasOverlay) {
        if (!candidate.overlay_.Restore(snapshot.overlay)) { lastError_ = candidate.overlay_.LastError(); return false; }
        candidate.hasOverlay_ = true;
        candidate.overlayStarted_ = true;
        candidate.overlayState_ = candidate.overlay_.ActiveStateId();
    } else {
        if (!candidate.overlay_.Reset()) return false;
        candidate.overlayStarted_ = false;
        candidate.overlayState_.clear();
    }
    base_ = std::move(candidate.base_);
    overlay_ = std::move(candidate.overlay_);
    baseState_ = std::move(candidate.baseState_);
    overlayState_ = std::move(candidate.overlayState_);
    hasBase_ = candidate.hasBase_;
    hasOverlay_ = candidate.hasOverlay_;
    baseStarted_ = candidate.baseStarted_;
    overlayStarted_ = candidate.overlayStarted_;
    lastError_ = AnimationStateMachineError::None;
    return true;
}

bool CharacterPawn::Snapshot(CharacterPawnSnapshot& snapshot) const {
    if (!attached_) return false;
    CharacterPawnSnapshot candidate{};
    candidate.actor = actor_;
    candidate.authority = lastAuthority_;
    candidate.rootMotionMode = rootMotionMode_;
    candidate.velocity = velocity_;
    candidate.grounded = grounded_;
    if (!animation_.Snapshot(candidate.animation)) return false;
    snapshot = std::move(candidate);
    return true;
}

bool CharacterPawn::Restore(const CharacterPawnSnapshot& snapshot) {
    if (!attached_ || snapshot.actor != actor_ || !Finite(snapshot.velocity.x) || !Finite(snapshot.velocity.y) || !Finite(snapshot.velocity.z) || (snapshot.rootMotionMode != CharacterRootMotionMode::Kinematic && snapshot.rootMotionMode != CharacterRootMotionMode::SkeletalRoot) || (snapshot.authority != CharacterMovementAuthority::None && snapshot.authority != CharacterMovementAuthority::KinematicRoute && snapshot.authority != CharacterMovementAuthority::SkeletalRoot)) return Fail(CharacterPawnError::AnimationRejected);
    CharacterAnimationGraph candidateAnimation = animation_;
    if (!candidateAnimation.Restore(snapshot.animation)) return Fail(CharacterPawnError::AnimationRejected);
    animation_ = std::move(candidateAnimation);
    rootMotionMode_ = snapshot.rootMotionMode;
    lastAuthority_ = snapshot.authority;
    velocity_ = snapshot.velocity;
    grounded_ = snapshot.grounded;
    lastError_ = CharacterPawnError::None;
    return true;
}

} // namespace NeoEngine
