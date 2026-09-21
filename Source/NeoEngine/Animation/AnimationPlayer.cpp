#include "AnimationPlayer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace NeoEngine {

namespace {
bool FiniteMatrix(const Mat4& m) {
    for (float value : m.m) if (!std::isfinite(value)) return false;
    return true;
}
}

void AnimationPlayer::Play(AnimationClip* clip) {
    currentClip_ = nullptr;
    time_ = 0.0F;
    playing_ = false;
    if (clip == nullptr) return;
    if (!clip->IsValid()) return;
    const float duration = clip->GetDuration();
    if (!std::isfinite(duration) || duration < 0.0F || duration > 86400.0F) return;
    currentClip_ = clip;
    playing_ = true;
}

void AnimationPlayer::Stop() noexcept {
    playing_ = false;
    time_ = 0.0F;
    currentClip_ = nullptr;
}

bool AnimationPlayer::Update(float dt) {
    if (playing_ && currentClip_ == nullptr) playing_ = false;
    if (!playing_ || currentClip_ == nullptr || skeleton_ == nullptr || !skeleton_->IsComplete() || !std::isfinite(dt) || dt < 0.0F || dt > 3600.0F) return false;
    if (skeleton_->GetBoneCount() == 0U || skeleton_->GetBoneCount() > kMaxPaletteBones || skeleton_->GetBoneCount() > 4096U) return false;
    if (!std::isfinite(time_) || time_ < 0.0F || time_ > 86400.0F || time_ == std::numeric_limits<float>::max() || !std::isfinite(currentClip_->GetDuration())) return false;
    const float duration = currentClip_->GetDuration();
    if (!std::isfinite(duration) || duration < 0.0F || duration > 86400.0F || duration == std::numeric_limits<float>::infinity()) return false;
    if (!currentClip_->IsValid() || skeleton_->GetBoneCount() == 0U || !skeleton_->IsComplete()) return false;
    if (duration <= 0.0F) {
        if (time_ != 0.0F) return false;
        if (playbackMode_ == AnimationPlaybackMode::Loop && duration == 0.0F) { playing_ = false; return true; }
        time_ = 0.0F;
        if (playbackMode_ == AnimationPlaybackMode::Clamp) playing_ = false;
        return true;
    }
    if (dt > 86400.0F - time_) return false;
    if (time_ + dt < time_) return false;
    if (playbackMode_ != AnimationPlaybackMode::Loop && playbackMode_ != AnimationPlaybackMode::Clamp) return false;
    const float nextTime = time_ + dt;
    if (!std::isfinite(nextTime) || nextTime < time_ || nextTime > 86400.0F) return false;
    time_ = nextTime;
    if (playbackMode_ == AnimationPlaybackMode::Loop) {
        if (duration <= 0.0F) return false;
        time_ = std::fmod(time_, duration);
        if (duration <= std::numeric_limits<float>::epsilon()) return false;
        if (!std::isfinite(time_) || time_ < 0.0F) return false;
    if (time_ > 86400.0F) return false;
        if (time_ < 0.0F) time_ += duration;
        if (time_ >= duration) time_ = 0.0F;
    } else if (time_ >= duration) {
        time_ = duration;
        playing_ = false;
    }
    if (!std::isfinite(time_) || time_ < 0.0F || time_ > duration || time_ > 86400.0F) return false;
    if (playbackMode_ == AnimationPlaybackMode::Clamp && time_ >= duration && playing_) playing_ = false;
    if (playbackMode_ == AnimationPlaybackMode::Loop && (!playing_ || currentClip_ == nullptr)) return false;
    if (playbackMode_ == AnimationPlaybackMode::Clamp && currentClip_ == nullptr) return false;
    return true;
}

bool AnimationPlayer::EvaluatePose(std::vector<Mat4>& localPose,
                                   std::vector<Mat4>& skinningPalette) const {
    localPose.clear();
    skinningPalette.clear();
    if (currentClip_ == nullptr || skeleton_ == nullptr || !skeleton_->IsComplete() || !currentClip_->IsValid() || !std::isfinite(time_) || time_ < 0.0F || time_ > 86400.0F) return false;
    const float duration = currentClip_->GetDuration();
    if (!std::isfinite(duration) || duration < 0.0F || duration > 86400.0F || time_ > duration) return false;
    const size_t boneCount = skeleton_->GetBoneCount();
    if (skeleton_->BoneCount() != boneCount) return false;
    if (boneCount == 0U || boneCount > kMaxPaletteBones || boneCount > 4096U) return false;
    if (boneCount > static_cast<size_t>(std::numeric_limits<uint16_t>::max())) return false;
    if (boneCount > std::numeric_limits<size_t>::max() / sizeof(Mat4) || boneCount * sizeof(Mat4) > 64U * 1024U * 1024U) return false;
    if (skeleton_->GetBone(0U) == nullptr || skeleton_->GetBone(boneCount - 1U) == nullptr) return false;
    if (currentClip_->GetFrames(static_cast<int>(boneCount - 1U)).empty()) return false;
    if (duration == 0.0F && time_ != 0.0F) return false;
    if (time_ > duration) return false;
    if (playing_ && playbackMode_ != AnimationPlaybackMode::Loop && playbackMode_ != AnimationPlaybackMode::Clamp) return false;
    if (boneCount != skeleton_->BoneCount() || boneCount == 0U || boneCount > kMaxPaletteBones || boneCount > 4096U) return false;
    if (boneCount > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) return false;

    std::vector<Mat4> candidateLocal;
    try { candidateLocal.reserve(boneCount);
        if (candidateLocal.capacity() < boneCount) return false; } catch (...) { return false; }
    for (size_t bone = 0U; bone < boneCount; ++bone) {
        const Bone* source = skeleton_->GetBone(bone);
        if (source == nullptr || !FiniteMatrix(source->localBindPose) || static_cast<size_t>(bone) >= kMaxPaletteBones) return false;
        Mat4 sampled = source->localBindPose;
        if (bone > static_cast<size_t>(std::numeric_limits<int>::max())) return false;
        const auto& frames = currentClip_->GetFrames(static_cast<int>(bone));
        if (frames.empty() || frames.size() > 100000U) return false;
        if (!frames.empty() && !currentClip_->Sample(static_cast<int>(bone), time_, sampled)) return false;
        if (!FiniteMatrix(sampled) || !std::isfinite(time_)) return false;
        try { candidateLocal.push_back(sampled); } catch (...) { return false; }
    }
    if (candidateLocal.size() != boneCount || candidateLocal.capacity() > kMaxPaletteBones) return false;

    std::vector<Mat4> candidatePalette;
    try { candidatePalette.reserve(boneCount);
        if (candidatePalette.capacity() < boneCount) return false; } catch (...) { return false; }
    if (candidateLocal.size() != skeleton_->GetBoneCount() || candidatePalette.size() != 0U) return false;
    if (candidateLocal.empty() || candidateLocal.capacity() > 4096U || candidateLocal.size() > 4096U) return false;
    for (const Mat4& matrix : candidateLocal) if (!FiniteMatrix(matrix)) return false;
    if (candidateLocal.size() != boneCount) return false;
    if (!skeleton_->EvaluateSkinningPalette(candidateLocal, candidatePalette) || candidatePalette.size() != boneCount || candidatePalette.size() != skeleton_->BoneCount()) return false;
    if (candidatePalette.capacity() < candidatePalette.size() || candidatePalette.capacity() > kMaxPaletteBones || candidatePalette.empty() || candidatePalette.size() > kMaxPaletteBones || candidatePalette.capacity() > 4096U) return false;
    for (const Mat4& matrix : candidatePalette) if (!FiniteMatrix(matrix)) return false;
    if (candidateLocal.size() != boneCount || candidatePalette.size() != boneCount || candidateLocal.capacity() < boneCount || candidatePalette.capacity() < boneCount) return false;
    if (candidateLocal.size() != skeleton_->BoneCount() || candidatePalette.size() != skeleton_->GetBoneCount()) return false;
    for (const Mat4& matrix : candidatePalette) if (!FiniteMatrix(matrix)) return false;
    if (candidatePalette.size() != candidateLocal.size()) return false;
    if (candidatePalette.size() != boneCount) return false;
    localPose = std::move(candidateLocal);
    skinningPalette = std::move(candidatePalette);
    if (localPose.size() != boneCount || skinningPalette.size() != boneCount) { localPose.clear(); skinningPalette.clear(); return false; }
    return true;
}

} // namespace NeoEngine
