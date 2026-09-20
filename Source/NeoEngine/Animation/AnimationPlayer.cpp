#include "AnimationPlayer.h"

#include <algorithm>
#include <cmath>

namespace NeoEngine {

void AnimationPlayer::Play(AnimationClip* clip) {
    currentClip_ = clip;
    time_ = 0.0F;
    playing_ = clip != nullptr;
}

void AnimationPlayer::Stop() noexcept {
    playing_ = false;
    time_ = 0.0F;
}

bool AnimationPlayer::Update(float dt) {
    if (!playing_ || currentClip_ == nullptr || !std::isfinite(dt) || dt < 0.0F) return false;
    const float duration = currentClip_->GetDuration();
    if (!std::isfinite(duration) || duration < 0.0F) return false;
    if (duration <= 0.0F) {
        time_ = 0.0F;
        if (playbackMode_ == AnimationPlaybackMode::Clamp) playing_ = false;
        return true;
    }

    time_ += dt;
    if (playbackMode_ == AnimationPlaybackMode::Loop) {
        time_ = std::fmod(time_, duration);
        if (time_ < 0.0F) time_ += duration;
    } else if (time_ >= duration) {
        time_ = duration;
        playing_ = false;
    }
    return true;
}

bool AnimationPlayer::EvaluatePose(std::vector<Mat4>& localPose,
                                   std::vector<Mat4>& skinningPalette) const {
    if (currentClip_ == nullptr || skeleton_ == nullptr || !skeleton_->IsComplete()) return false;
    const size_t boneCount = skeleton_->GetBoneCount();
    if (boneCount == 0U || boneCount > kMaxPaletteBones) return false;

    std::vector<Mat4> candidateLocal;
    candidateLocal.reserve(boneCount);
    for (size_t bone = 0U; bone < boneCount; ++bone) {
        const Bone* source = skeleton_->GetBone(bone);
        if (source == nullptr) return false;
        Mat4 sampled = source->localBindPose;
        const auto& frames = currentClip_->GetFrames(static_cast<int>(bone));
        if (!frames.empty() && !currentClip_->Sample(static_cast<int>(bone), time_, sampled)) return false;
        candidateLocal.push_back(sampled);
    }

    std::vector<Mat4> candidatePalette;
    if (!skeleton_->EvaluateSkinningPalette(candidateLocal, candidatePalette) ||
        candidatePalette.size() != boneCount) return false;

    localPose = std::move(candidateLocal);
    skinningPalette = std::move(candidatePalette);
    return true;
}

} // namespace NeoEngine
