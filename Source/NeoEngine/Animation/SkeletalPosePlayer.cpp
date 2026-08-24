#include "SkeletalPosePlayer.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace NeoEngine {
namespace {
bool Finite(const float value) { return std::isfinite(value); }
}

bool SkeletalPosePlayer::Bind(const SkeletalPoseClip& clip, const SkeletalPosePlaybackMode mode) {
    SkeletalPoseClip candidateClip = clip;
    std::vector<Mat4> candidatePose;
    if (!candidateClip.Sample(0.0F, candidatePose)) { lastError_ = SkeletalPosePlayerError::SampleFailed; return false; }
    if (mode == SkeletalPosePlaybackMode::Loop && candidateClip.Duration() <= 0.0F) { lastError_ = SkeletalPosePlayerError::LoopUnavailable; return false; }
    clip_ = std::move(candidateClip); hasClip_ = true; mode_ = mode; time_ = 0.0F; speed_ = 1.0F; paused_ = false; lastError_ = SkeletalPosePlayerError::None; return true;
}

bool SkeletalPosePlayer::SetSpeed(const float speed) {
    if (!Finite(speed) || speed < 0.0F || speed > kMaxSpeed) { lastError_ = SkeletalPosePlayerError::InvalidSpeed; return false; }
    speed_ = speed; lastError_ = SkeletalPosePlayerError::None; return true;
}

bool SkeletalPosePlayer::Advance(const float deltaSeconds, std::vector<Mat4>& output) {
    if (!hasClip_) { lastError_ = SkeletalPosePlayerError::NoClip; return false; }
    if (!Finite(deltaSeconds) || deltaSeconds < 0.0F || deltaSeconds > kMaxDeltaSeconds) { lastError_ = SkeletalPosePlayerError::InvalidDelta; return false; }
    const float advanced = paused_ ? time_ : time_ + deltaSeconds * speed_;
    if (!Finite(advanced)) { lastError_ = SkeletalPosePlayerError::InvalidDelta; return false; }
    float candidateTime = advanced;
    if (mode_ == SkeletalPosePlaybackMode::Loop) {
        const float duration = clip_.Duration();
        if (!Finite(duration) || duration <= 0.0F) { lastError_ = SkeletalPosePlayerError::LoopUnavailable; return false; }
        candidateTime = std::fmod(advanced, duration);
    } else candidateTime = std::min(advanced, clip_.Duration());
    std::vector<Mat4> candidateOutput;
    const bool sampled = mode_ == SkeletalPosePlaybackMode::Loop ? clip_.SampleLooped(candidateTime, candidateOutput) : clip_.Sample(candidateTime, candidateOutput);
    if (!sampled) { lastError_ = SkeletalPosePlayerError::SampleFailed; return false; }
    output.swap(candidateOutput); time_ = candidateTime; lastError_ = SkeletalPosePlayerError::None; return true;
}

} // namespace NeoEngine
