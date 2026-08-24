#include "SkeletalAnimationController.h"

#include <cmath>
#include <utility>

namespace NeoEngine {
namespace {
bool RootTranslation(const std::vector<Mat4>& localPose, RootMotionDelta& result) {
    if (localPose.empty()) return false;
    const Mat4& root = localPose.front();
    if (!std::isfinite(root.m[12]) || !std::isfinite(root.m[13]) || !std::isfinite(root.m[14])) return false;
    result = {root.m[12], root.m[13], root.m[14]}; return true;
}
}

bool SkeletalAnimationController::Initialize(const Skeleton& skeleton, const SkeletalPoseClip& clip, const SkeletalPosePlaybackMode mode) {
    if (skeleton.GetBoneCount() == 0U || skeleton.GetBoneCount() != clip.BoneCount()) { lastError_ = SkeletalAnimationControllerError::ClipSkeletonMismatch; return false; }
    Skeleton candidateSkeleton = skeleton;
    if (!candidateSkeleton.DeriveInverseBindPose()) { lastError_ = SkeletalAnimationControllerError::SkeletonInvalid; return false; }
    SkeletalPosePlayer candidatePlayer;
    if (!candidatePlayer.Bind(clip, mode)) { lastError_ = SkeletalAnimationControllerError::PlayerBindFailed; return false; }
    SkeletalPoseClip rootClip = clip;
    const float rootDuration = rootClip.Duration();
    std::vector<Mat4> initialPose, terminalPose; RootMotionDelta initialRoot{}, terminalRoot{};
    if (!candidatePlayer.Advance(0.0F, initialPose) || !rootClip.Sample(0.0F, initialPose) || !RootTranslation(initialPose, initialRoot) || !rootClip.Sample(rootDuration, terminalPose) || !RootTranslation(terminalPose, terminalRoot)) { lastError_ = SkeletalAnimationControllerError::RootMotionInvalid; return false; }
    skeleton_ = std::move(candidateSkeleton); player_ = std::move(candidatePlayer); mode_ = mode; rootStartTranslation_ = initialRoot; rootEndTranslation_ = terminalRoot; rootTranslation_ = initialRoot; rootCycleTime_ = 0.0F; rootLoopDuration_ = rootDuration; initialized_ = true; lastError_ = SkeletalAnimationControllerError::None; return true;
}

bool SkeletalAnimationController::SetSpeed(const float speed) {
    if (!initialized_) { lastError_ = SkeletalAnimationControllerError::NotInitialized; return false; }
    if (!player_.SetSpeed(speed)) { lastError_ = SkeletalAnimationControllerError::PlayerAdvanceFailed; return false; }
    lastError_ = SkeletalAnimationControllerError::None; return true;
}

bool SkeletalAnimationController::Advance(const float deltaSeconds, std::vector<Mat4>& palette) {
    if (!initialized_) { lastError_ = SkeletalAnimationControllerError::NotInitialized; return false; }
    SkeletalPosePlayer candidatePlayer = player_;
    std::vector<Mat4> localPose;
    if (!candidatePlayer.Advance(deltaSeconds, localPose)) { lastError_ = SkeletalAnimationControllerError::PlayerAdvanceFailed; return false; }
    RootMotionDelta candidateRoot{};
    if (!RootTranslation(localPose, candidateRoot)) { lastError_ = SkeletalAnimationControllerError::RootMotionInvalid; return false; }
    std::vector<Mat4> candidatePalette;
    if (!skeleton_.EvaluateSkinningPalette(localPose, candidatePalette)) { lastError_ = SkeletalAnimationControllerError::PaletteFailed; return false; }
    player_ = std::move(candidatePlayer); rootTranslation_ = candidateRoot; rootCycleTime_ = player_.Time(); palette.swap(candidatePalette); lastError_ = SkeletalAnimationControllerError::None; return true;
}

bool SkeletalAnimationController::AdvanceAndSkin(const float deltaSeconds, std::vector<float>& positions, std::vector<float>& normals, const std::vector<VertexWeight>& weights) {
    if (!initialized_) { lastError_ = SkeletalAnimationControllerError::NotInitialized; return false; }
    SkeletalPosePlayer candidatePlayer = player_;
    std::vector<Mat4> localPose;
    if (!candidatePlayer.Advance(deltaSeconds, localPose)) { lastError_ = SkeletalAnimationControllerError::PlayerAdvanceFailed; return false; }
    RootMotionDelta candidateRoot{};
    if (!RootTranslation(localPose, candidateRoot)) { lastError_ = SkeletalAnimationControllerError::RootMotionInvalid; return false; }
    std::vector<Mat4> candidatePalette;
    if (!skeleton_.EvaluateSkinningPalette(localPose, candidatePalette)) { lastError_ = SkeletalAnimationControllerError::PaletteFailed; return false; }
    std::vector<float> candidatePositions = positions;
    std::vector<float> candidateNormals = normals;
    SkinningError skinningError = SkinningError::None;
    if (!Skinning::ApplySkinningWithNormals(candidatePositions, candidateNormals, weights, candidatePalette, &skinningError)) { lastError_ = SkeletalAnimationControllerError::SkinningFailed; return false; }
    player_ = std::move(candidatePlayer); rootTranslation_ = candidateRoot; rootCycleTime_ = player_.Time(); positions.swap(candidatePositions); normals.swap(candidateNormals); lastError_ = SkeletalAnimationControllerError::None; return true;
}

bool SkeletalAnimationController::AdvanceWithRootMotion(const float deltaSeconds, std::vector<Mat4>& palette, RootMotionDelta& rootMotion) {
    if (!initialized_) { lastError_ = SkeletalAnimationControllerError::NotInitialized; return false; }
    SkeletalPosePlayer candidatePlayer = player_;
    std::vector<Mat4> localPose;
    if (!candidatePlayer.Advance(deltaSeconds, localPose)) { lastError_ = SkeletalAnimationControllerError::PlayerAdvanceFailed; return false; }
    RootMotionDelta candidateRoot{};
    if (!RootTranslation(localPose, candidateRoot)) { lastError_ = SkeletalAnimationControllerError::RootMotionInvalid; return false; }
    std::vector<Mat4> candidatePalette;
    if (!skeleton_.EvaluateSkinningPalette(localPose, candidatePalette)) { lastError_ = SkeletalAnimationControllerError::PaletteFailed; return false; }
    const RootMotionDelta candidateDelta{candidateRoot.x - rootTranslation_.x, candidateRoot.y - rootTranslation_.y, candidateRoot.z - rootTranslation_.z};
    RootMotionDelta accumulatedDelta = candidateDelta;
    float candidateCycleTime = candidatePlayer.Time();
    if (mode_ == SkeletalPosePlaybackMode::Loop) {
        if (!std::isfinite(rootLoopDuration_) || rootLoopDuration_ <= 0.0F) { lastError_ = SkeletalAnimationControllerError::RootMotionInvalid; return false; }
        const float rawAdvance = candidatePlayer.IsPaused() ? 0.0F : deltaSeconds * candidatePlayer.Speed();
        const float totalTime = rootCycleTime_ + rawAdvance;
        if (!std::isfinite(rawAdvance) || !std::isfinite(totalTime)) { lastError_ = SkeletalAnimationControllerError::RootMotionInvalid; return false; }
        const int wraps = static_cast<int>(std::floor(totalTime / rootLoopDuration_));
        if (wraps < 0 || wraps > 128) { lastError_ = SkeletalAnimationControllerError::RootMotionWrapExceeded; return false; }
        candidateCycleTime = std::fmod(totalTime, rootLoopDuration_);
        if (candidateCycleTime < 0.0F) candidateCycleTime += rootLoopDuration_;
        if (wraps > 0) {
            const RootMotionDelta cycle{rootEndTranslation_.x - rootStartTranslation_.x, rootEndTranslation_.y - rootStartTranslation_.y, rootEndTranslation_.z - rootStartTranslation_.z};
            accumulatedDelta = {rootEndTranslation_.x - rootTranslation_.x + static_cast<float>(wraps - 1) * cycle.x + candidateRoot.x - rootStartTranslation_.x, rootEndTranslation_.y - rootTranslation_.y + static_cast<float>(wraps - 1) * cycle.y + candidateRoot.y - rootStartTranslation_.y, rootEndTranslation_.z - rootTranslation_.z + static_cast<float>(wraps - 1) * cycle.z + candidateRoot.z - rootStartTranslation_.z};
        }
    }
    player_ = std::move(candidatePlayer); rootTranslation_ = candidateRoot; rootCycleTime_ = candidateCycleTime; palette.swap(candidatePalette); rootMotion = accumulatedDelta; lastError_ = SkeletalAnimationControllerError::None; return true;
}

bool SkeletalAnimationController::AdvanceApplyRootMotion(const float deltaSeconds, SceneWorld& world, const SceneEntity entity, std::vector<Mat4>& palette) {
    if (!initialized_) { lastError_ = SkeletalAnimationControllerError::NotInitialized; return false; }
    SkeletalPosePlayer candidatePlayer = player_;
    std::vector<Mat4> localPose;
    if (!candidatePlayer.Advance(deltaSeconds, localPose)) { lastError_ = SkeletalAnimationControllerError::PlayerAdvanceFailed; return false; }
    RootMotionDelta candidateRoot{};
    if (!RootTranslation(localPose, candidateRoot)) { lastError_ = SkeletalAnimationControllerError::RootMotionInvalid; return false; }
    localPose[0].m[12] -= candidateRoot.x - rootStartTranslation_.x;
    localPose[0].m[13] -= candidateRoot.y - rootStartTranslation_.y;
    localPose[0].m[14] -= candidateRoot.z - rootStartTranslation_.z;
    std::vector<Mat4> candidatePalette;
    if (!skeleton_.EvaluateSkinningPalette(localPose, candidatePalette)) { lastError_ = SkeletalAnimationControllerError::PaletteFailed; return false; }
    RootMotionDelta candidateDelta{candidateRoot.x - rootTranslation_.x, candidateRoot.y - rootTranslation_.y, candidateRoot.z - rootTranslation_.z};
    float candidateCycleTime = candidatePlayer.Time();
    if (mode_ == SkeletalPosePlaybackMode::Loop) {
        if (!std::isfinite(rootLoopDuration_) || rootLoopDuration_ <= 0.0F) { lastError_ = SkeletalAnimationControllerError::RootMotionInvalid; return false; }
        const float rawAdvance = candidatePlayer.IsPaused() ? 0.0F : deltaSeconds * candidatePlayer.Speed();
        const float totalTime = rootCycleTime_ + rawAdvance;
        if (!std::isfinite(rawAdvance) || !std::isfinite(totalTime)) { lastError_ = SkeletalAnimationControllerError::RootMotionInvalid; return false; }
        const int wraps = static_cast<int>(std::floor(totalTime / rootLoopDuration_));
        if (wraps < 0 || wraps > 128) { lastError_ = SkeletalAnimationControllerError::RootMotionWrapExceeded; return false; }
        candidateCycleTime = std::fmod(totalTime, rootLoopDuration_);
        if (candidateCycleTime < 0.0F) candidateCycleTime += rootLoopDuration_;
        if (wraps > 0) {
            const RootMotionDelta cycle{rootEndTranslation_.x - rootStartTranslation_.x, rootEndTranslation_.y - rootStartTranslation_.y, rootEndTranslation_.z - rootStartTranslation_.z};
            candidateDelta = {rootEndTranslation_.x - rootTranslation_.x + static_cast<float>(wraps - 1) * cycle.x + candidateRoot.x - rootStartTranslation_.x, rootEndTranslation_.y - rootTranslation_.y + static_cast<float>(wraps - 1) * cycle.y + candidateRoot.y - rootStartTranslation_.y, rootEndTranslation_.z - rootTranslation_.z + static_cast<float>(wraps - 1) * cycle.z + candidateRoot.z - rootStartTranslation_.z};
        }
    }
    SceneWorld candidateWorld = world;
    const Transform3* localTransform = candidateWorld.GetLocalTransform(entity);
    if (localTransform == nullptr) { lastError_ = SkeletalAnimationControllerError::SceneApplyFailed; return false; }
    Transform3 updated = *localTransform;
    updated.x += candidateDelta.x;
    updated.y += candidateDelta.y;
    updated.z += candidateDelta.z;
    if (!candidateWorld.SetTransform(entity, updated)) { lastError_ = SkeletalAnimationControllerError::SceneApplyFailed; return false; }
    world = std::move(candidateWorld); player_ = std::move(candidatePlayer); rootTranslation_ = candidateRoot; rootCycleTime_ = candidateCycleTime; palette.swap(candidatePalette); lastError_ = SkeletalAnimationControllerError::None; return true;
}

bool SkeletalAnimationController::AdvanceApplyRootMotionGuarded(const float deltaSeconds, SceneWorld& world, const SceneEntity entity, std::vector<Mat4>& palette, MovementAuthorityGate& authority) {
    MovementAuthorityGate candidateAuthority = authority;
    if (!candidateAuthority.Acquire(entity, MovementAuthority::SkeletalRoot)) { lastError_ = SkeletalAnimationControllerError::AuthorityConflict; return false; }
    if (!AdvanceApplyRootMotion(deltaSeconds, world, entity, palette)) return false;
    authority = std::move(candidateAuthority);
    return true;
}

} // namespace NeoEngine
