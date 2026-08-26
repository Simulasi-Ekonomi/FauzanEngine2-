#include "CharacterPawn.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>

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
bool WriteU16(std::span<uint8_t> bytes, size_t& offset, uint16_t value) {
    if (offset > bytes.size() || bytes.size() - offset < 2U) return false;
    bytes[offset++] = static_cast<uint8_t>(value & 0xFFU); bytes[offset++] = static_cast<uint8_t>(value >> 8U); return true;
}
bool WriteU32(std::span<uint8_t> bytes, size_t& offset, uint32_t value) {
    if (offset > bytes.size() || bytes.size() - offset < 4U) return false;
    for (uint8_t shift = 0U; shift < 32U; shift += 8U) bytes[offset++] = static_cast<uint8_t>(value >> shift); return true;
}
bool WriteFloat(std::span<uint8_t> bytes, size_t& offset, float value) {
    uint32_t raw = 0U; std::memcpy(&raw, &value, sizeof(raw)); return WriteU32(bytes, offset, raw);
}
bool WriteString(std::span<uint8_t> bytes, size_t& offset, std::string_view value) {
    if (value.size() > AnimationStateMachine::kMaxIdentifierBytes || value.find('\0') != std::string_view::npos || offset >= bytes.size() || value.size() > 255U || bytes.size() - offset < value.size() + 1U) return false;
    bytes[offset++] = static_cast<uint8_t>(value.size()); std::memcpy(bytes.data() + offset, value.data(), value.size()); offset += value.size(); return true;
}
bool ReadU16(std::span<const uint8_t> bytes, size_t& offset, uint16_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < 2U) return false;
    value = static_cast<uint16_t>(bytes[offset]) | static_cast<uint16_t>(bytes[offset + 1U]) << 8U; offset += 2U; return true;
}
bool ReadU32(std::span<const uint8_t> bytes, size_t& offset, uint32_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < 4U) return false;
    value = 0U; for (uint8_t shift = 0U; shift < 32U; shift += 8U) value |= static_cast<uint32_t>(bytes[offset++]) << shift; return true;
}
bool ReadFloat(std::span<const uint8_t> bytes, size_t& offset, float& value) {
    uint32_t raw = 0U; if (!ReadU32(bytes, offset, raw)) return false; std::memcpy(&value, &raw, sizeof(value)); return true;
}
bool ReadString(std::span<const uint8_t> bytes, size_t& offset, std::string& value) {
    if (offset >= bytes.size()) return false; const uint8_t length = bytes[offset++];
    if (length > AnimationStateMachine::kMaxIdentifierBytes || bytes.size() - offset < length) return false;
    for (uint8_t index = 0U; index < length; ++index) if (bytes[offset + index] == 0U) return false;
    value.assign(reinterpret_cast<const char*>(bytes.data() + offset), length); offset += length; return true;
}
bool WriteStateSnapshot(std::span<uint8_t> bytes, size_t& offset, const AnimationStateMachineSnapshot& snapshot) {
    if (!WriteString(bytes, offset, snapshot.activeStateId) || !WriteString(bytes, offset, snapshot.targetStateId) || !WriteString(bytes, offset, snapshot.transitionId) || offset >= bytes.size()) return false;
    bytes[offset++] = snapshot.blending ? 1U : 0U;
    return WriteFloat(bytes, offset, snapshot.blendFraction) && WriteFloat(bytes, offset, snapshot.activeTimeSeconds) && WriteFloat(bytes, offset, snapshot.targetTimeSeconds);
}
bool ReadStateSnapshot(std::span<const uint8_t> bytes, size_t& offset, AnimationStateMachineSnapshot& snapshot) {
    if (!ReadString(bytes, offset, snapshot.activeStateId) || !ReadString(bytes, offset, snapshot.targetStateId) || !ReadString(bytes, offset, snapshot.transitionId) || offset >= bytes.size() || bytes[offset] > 1U) return false;
    snapshot.blending = bytes[offset++] != 0U; return ReadFloat(bytes, offset, snapshot.blendFraction) && ReadFloat(bytes, offset, snapshot.activeTimeSeconds) && ReadFloat(bytes, offset, snapshot.targetTimeSeconds);
}
bool EncodeCharacterSnapshot(const CharacterPawnSnapshot& snapshot, std::span<uint8_t> bytes) {
    std::fill(bytes.begin(), bytes.end(), 0U); size_t offset = 0U;
    if (!WriteU16(bytes, offset, snapshot.actor.index) || !WriteU16(bytes, offset, snapshot.actor.generation) || offset + 2U > bytes.size()) return false;
    bytes[offset++] = static_cast<uint8_t>(snapshot.authority); bytes[offset++] = static_cast<uint8_t>(snapshot.rootMotionMode);
    if (!WriteFloat(bytes, offset, snapshot.velocity.x) || !WriteFloat(bytes, offset, snapshot.velocity.y) || !WriteFloat(bytes, offset, snapshot.velocity.z) || !WriteFloat(bytes, offset, snapshot.pendingInput.moveX) || !WriteFloat(bytes, offset, snapshot.pendingInput.moveZ) || offset + 2U > bytes.size()) return false;
    bytes[offset++] = snapshot.grounded ? 1U : 0U; bytes[offset++] = snapshot.pendingInput.sprint ? 1U : 0U; bytes[offset++] = snapshot.pendingInput.jump ? 1U : 0U;
    if (!WriteFloat(bytes, offset, snapshot.pendingRootMotion.x) || !WriteFloat(bytes, offset, snapshot.pendingRootMotion.y) || !WriteFloat(bytes, offset, snapshot.pendingRootMotion.z) || !WriteStateSnapshot(bytes, offset, snapshot.animation.base) || !WriteStateSnapshot(bytes, offset, snapshot.animation.overlay) || !WriteU16(bytes, offset, snapshot.animation.overlayWeightPermille) || offset >= bytes.size()) return false;
    bytes[offset++] = snapshot.animation.hasOverlay ? 1U : 0U; return true;
}
bool DecodeCharacterSnapshot(std::span<const uint8_t> bytes, CharacterPawnSnapshot& snapshot) {
    size_t offset = 0U; uint16_t index = 0U; uint16_t generation = 0U;
    if (bytes.size() != CharacterPawn::kComponentSnapshotBytes || !ReadU16(bytes, offset, index) || !ReadU16(bytes, offset, generation) || offset + 2U > bytes.size()) return false;
    snapshot.actor = {index, generation}; snapshot.authority = static_cast<CharacterMovementAuthority>(bytes[offset++]); snapshot.rootMotionMode = static_cast<CharacterRootMotionMode>(bytes[offset++]);
    if (!ReadFloat(bytes, offset, snapshot.velocity.x) || !ReadFloat(bytes, offset, snapshot.velocity.y) || !ReadFloat(bytes, offset, snapshot.velocity.z) || !ReadFloat(bytes, offset, snapshot.pendingInput.moveX) || !ReadFloat(bytes, offset, snapshot.pendingInput.moveZ) || offset + 2U > bytes.size() || bytes[offset] > 1U || bytes[offset + 1U] > 1U || bytes[offset + 2U] > 1U) return false;
    snapshot.grounded = bytes[offset++] != 0U; snapshot.pendingInput.sprint = bytes[offset++] != 0U; snapshot.pendingInput.jump = bytes[offset++] != 0U;
    if (!ReadFloat(bytes, offset, snapshot.pendingRootMotion.x) || !ReadFloat(bytes, offset, snapshot.pendingRootMotion.y) || !ReadFloat(bytes, offset, snapshot.pendingRootMotion.z) || !ReadStateSnapshot(bytes, offset, snapshot.animation.base) || !ReadStateSnapshot(bytes, offset, snapshot.animation.overlay) || !ReadU16(bytes, offset, snapshot.animation.overlayWeightPermille) || offset >= bytes.size() || bytes[offset] > 1U) return false;
    snapshot.animation.hasOverlay = bytes[offset++] != 0U; return true;
}
bool ValidCharacterSnapshot(const CharacterPawnSnapshot& snapshot, SceneEntity actor, const CharacterPawnConfig& config, const CharacterAnimationGraph& currentAnimation) {
    const float inputMagnitudeSquared = snapshot.pendingInput.moveX * snapshot.pendingInput.moveX + snapshot.pendingInput.moveZ * snapshot.pendingInput.moveZ;
    if (snapshot.actor != actor || (snapshot.authority != CharacterMovementAuthority::None && snapshot.authority != CharacterMovementAuthority::KinematicRoute && snapshot.authority != CharacterMovementAuthority::SkeletalRoot) || (snapshot.rootMotionMode != CharacterRootMotionMode::Kinematic && snapshot.rootMotionMode != CharacterRootMotionMode::SkeletalRoot) || !Finite(snapshot.velocity.x) || !Finite(snapshot.velocity.y) || !Finite(snapshot.velocity.z) || (snapshot.grounded && std::abs(snapshot.velocity.y) > 0.0001F) || !Finite(inputMagnitudeSquared) || inputMagnitudeSquared > config.maxPlanarInput * config.maxPlanarInput || !ValidRootMotion(snapshot.pendingRootMotion) || snapshot.animation.overlayWeightPermille > 1000U) return false;
    CharacterAnimationGraph candidate = currentAnimation; return candidate.Restore(snapshot.animation);
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
    CharacterAnimationGraph candidate = *this;
    if (candidate.baseStarted_ && !candidate.base_.Update(deltaSeconds)) { lastError_ = candidate.base_.LastError(); return false; }
    if (candidate.overlayStarted_ && !candidate.overlay_.Update(deltaSeconds)) { lastError_ = candidate.overlay_.LastError(); return false; }
    if (candidate.baseStarted_) candidate.baseState_ = candidate.base_.ActiveStateId();
    if (candidate.overlayStarted_) candidate.overlayState_ = candidate.overlay_.ActiveStateId();
    base_ = std::move(candidate.base_);
    overlay_ = std::move(candidate.overlay_);
    baseState_ = std::move(candidate.baseState_);
    overlayState_ = std::move(candidate.overlayState_);
    lastError_ = AnimationStateMachineError::None;
    return true;
}

bool CharacterAnimationGraph::SetOverlayWeightPermille(uint16_t weightPermille) {
    if (weightPermille > 1000U) { lastError_ = AnimationStateMachineError::InvalidState; return false; }
    overlayWeightPermille_ = weightPermille;
    lastError_ = AnimationStateMachineError::None;
    return true;
}

bool CharacterAnimationGraph::Sample(const AnimationTimeline& timeline, float& value) const {
    float baseValue = 0.0F;
    if (!baseStarted_ || !base_.Sample(timeline, baseValue)) { lastError_ = base_.LastError(); return false; }
    if (!overlayStarted_ || overlayWeightPermille_ == 0U) { value = baseValue; lastError_ = AnimationStateMachineError::None; return true; }
    float overlayValue = 0.0F;
    if (!overlay_.Sample(timeline, overlayValue)) { lastError_ = overlay_.LastError(); return false; }
    const float weight = static_cast<float>(overlayWeightPermille_) / 1000.0F;
    value = baseValue + (overlayValue - baseValue) * weight;
    lastError_ = std::isfinite(value) ? AnimationStateMachineError::None : AnimationStateMachineError::SampleFailed;
    return lastError_ == AnimationStateMachineError::None;
}
bool CharacterAnimationGraph::CollectAnimationEvents(const AnimationTimeline& timeline, float fromTime, float toTime, std::vector<std::string>& output) const {
    if (!baseStarted_) { lastError_ = AnimationStateMachineError::NotStarted; return false; }
    std::vector<std::string> candidate;
    if (!base_.CollectEvents(timeline, fromTime, toTime, candidate)) { lastError_ = base_.LastError(); return false; }
    if (overlayStarted_ && overlayWeightPermille_ != 0U) {
        std::vector<std::string> overlayEvents;
        if (!overlay_.CollectEvents(timeline, fromTime, toTime, overlayEvents) || candidate.size() > 256U || overlayEvents.size() > 256U - candidate.size()) { lastError_ = AnimationStateMachineError::EventCollectionFailed; return false; }
        candidate.insert(candidate.end(), overlayEvents.begin(), overlayEvents.end());
    }
    output = std::move(candidate);
    lastError_ = AnimationStateMachineError::None;
    return true;
}

bool CharacterAnimationGraph::Snapshot(CharacterAnimationGraphSnapshot& snapshot) const {
    try {
        CharacterAnimationGraphSnapshot candidate{};
        if (baseStarted_ && !base_.Snapshot(candidate.base)) { lastError_ = base_.LastError(); return false; }
        candidate.hasOverlay = hasOverlay_ && overlayStarted_;
        candidate.overlayWeightPermille = overlayWeightPermille_;
        if (candidate.hasOverlay && !overlay_.Snapshot(candidate.overlay)) { lastError_ = overlay_.LastError(); return false; }
        snapshot = std::move(candidate);
        lastError_ = AnimationStateMachineError::None;
        return true;
    } catch (const std::bad_alloc&) {
        lastError_ = AnimationStateMachineError::Capacity;
        return false;
    }
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
    if (binding.from.empty() || binding.to.empty() || binding.transitionId.empty() || binding.from.size() > AnimationStateMachine::kMaxIdentifierBytes || binding.to.size() > AnimationStateMachine::kMaxIdentifierBytes || binding.transitionId.size() > AnimationStateMachine::kMaxIdentifierBytes) return Fail(CharacterPawnError::AnimationRejected);
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
bool CharacterPawn::CaptureSnapshot(std::span<uint8_t> bytes) const {
    if (bytes.size() != kComponentSnapshotBytes) return false;
    CharacterPawnSnapshot snapshot{}; std::array<uint8_t, kComponentSnapshotBytes> candidate{};
    if (!Snapshot(snapshot) || !EncodeCharacterSnapshot(snapshot, candidate)) return false;
    std::copy(candidate.begin(), candidate.end(), bytes.begin()); return true;
}
bool CharacterPawn::ValidateSnapshot(std::span<const uint8_t> bytes) const {
    if (!attached_ || bytes.size() != kComponentSnapshotBytes) return false;
    CharacterPawnSnapshot snapshot{}; return DecodeCharacterSnapshot(bytes, snapshot) && ValidCharacterSnapshot(snapshot, actor_, config_, animation_);
}
bool CharacterPawn::RestoreSnapshot(std::span<const uint8_t> bytes) {
    if (!attached_ || bytes.size() != kComponentSnapshotBytes) return false;
    CharacterPawnSnapshot snapshot{};
    if (!DecodeCharacterSnapshot(bytes, snapshot) || !ValidCharacterSnapshot(snapshot, actor_, config_, animation_)) return false;
    return Restore(snapshot);
}
bool CharacterPawn::CollectAnimationEvents(const AnimationTimeline& timeline, float fromTime, float toTime, std::vector<std::string>& output) const {
    if (!attached_) return false;
    return animation_.CollectAnimationEvents(timeline, fromTime, toTime, output);
}

bool CharacterPawn::TriggerOverlay(std::string_view transitionId) {
    if (!attached_) return Fail(CharacterPawnError::NotInitialized);
    if (!animation_.TriggerOverlay(transitionId)) return Fail(CharacterPawnError::AnimationRejected);
    lastError_ = CharacterPawnError::None;
    return true;
}

bool CharacterAnimationGraph::Restore(const CharacterAnimationGraphSnapshot& snapshot) {
    if (snapshot.overlayWeightPermille > 1000U) { lastError_ = AnimationStateMachineError::InvalidSnapshot; return false; }
    try {
        CharacterAnimationGraph candidate = *this;
        candidate.overlayWeightPermille_ = snapshot.overlayWeightPermille;
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
            candidate.hasOverlay_ = false;
            candidate.overlayStarted_ = false;
            candidate.overlayState_.clear();
        }
        base_ = std::move(candidate.base_);
        overlay_ = std::move(candidate.overlay_);
        baseState_ = std::move(candidate.baseState_);
        overlayState_ = std::move(candidate.overlayState_);
        overlayWeightPermille_ = candidate.overlayWeightPermille_;
        hasBase_ = candidate.hasBase_;
        hasOverlay_ = candidate.hasOverlay_;
        baseStarted_ = candidate.baseStarted_;
        overlayStarted_ = candidate.overlayStarted_;
        lastError_ = AnimationStateMachineError::None;
        return true;
    } catch (const std::bad_alloc&) {
        lastError_ = AnimationStateMachineError::Capacity;
        return false;
    }
}

bool CharacterPawn::Snapshot(CharacterPawnSnapshot& snapshot) const {
    if (!attached_) return false;
    try {
        CharacterPawnSnapshot candidate{};
        candidate.actor = actor_;
        candidate.authority = lastAuthority_;
        candidate.rootMotionMode = rootMotionMode_;
        candidate.velocity = velocity_;
        candidate.grounded = grounded_;
        candidate.pendingInput = pendingInput_;
        candidate.pendingRootMotion = pendingRootMotion_;
        if (!animation_.Snapshot(candidate.animation)) return false;
        snapshot = std::move(candidate);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

bool CharacterPawn::Restore(const CharacterPawnSnapshot& snapshot) {
    if (!attached_ || snapshot.actor != actor_ || !Finite(snapshot.velocity.x) || !Finite(snapshot.velocity.y) || !Finite(snapshot.velocity.z) || (snapshot.grounded && std::abs(snapshot.velocity.y) > 0.0001F) || !ValidateInput(snapshot.pendingInput) || !ValidRootMotion(snapshot.pendingRootMotion) || (snapshot.rootMotionMode != CharacterRootMotionMode::Kinematic && snapshot.rootMotionMode != CharacterRootMotionMode::SkeletalRoot) || (snapshot.authority != CharacterMovementAuthority::None && snapshot.authority != CharacterMovementAuthority::KinematicRoute && snapshot.authority != CharacterMovementAuthority::SkeletalRoot)) return Fail(CharacterPawnError::AnimationRejected);
    try {
        CharacterAnimationGraph candidateAnimation = animation_;
        if (!candidateAnimation.Restore(snapshot.animation)) return Fail(CharacterPawnError::AnimationRejected);
        animation_ = std::move(candidateAnimation);
        rootMotionMode_ = snapshot.rootMotionMode;
        lastAuthority_ = snapshot.authority;
        velocity_ = snapshot.velocity;
        grounded_ = snapshot.grounded;
        pendingInput_ = snapshot.pendingInput;
        pendingRootMotion_ = snapshot.pendingRootMotion;
        lastError_ = CharacterPawnError::None;
        return true;
    } catch (const std::bad_alloc&) {
        return Fail(CharacterPawnError::AnimationRejected);
    }
}

} // namespace NeoEngine
