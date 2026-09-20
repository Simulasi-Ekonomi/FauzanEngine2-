#pragma once

#include "Skeleton.h"
#include "AnimationClip.h"

#include <cstddef>
#include <vector>

namespace NeoEngine {

enum class AnimationPlaybackMode : unsigned char { Clamp, Loop };

class AnimationPlayer {
public:
    static constexpr size_t kMaxPaletteBones = Skeleton::kMaxBones;

    void Play(AnimationClip* clip);
    void Stop() noexcept;
    void SetPlaybackMode(AnimationPlaybackMode mode) noexcept { playbackMode_ = mode; }
    void SetSkeleton(const Skeleton* skeleton) noexcept { skeleton_ = skeleton; }
    bool Update(float dt);
    bool EvaluatePose(std::vector<Mat4>& localPose, std::vector<Mat4>& skinningPalette) const;

    [[nodiscard]] bool IsPlaying() const noexcept { return currentClip_ != nullptr && playing_; }
    [[nodiscard]] float GetTime() const noexcept { return time_; }
    [[nodiscard]] AnimationClip* GetCurrentClip() const noexcept { return currentClip_; }
    [[nodiscard]] AnimationPlaybackMode GetPlaybackMode() const noexcept { return playbackMode_; }

private:
    AnimationClip* currentClip_ = nullptr;
    const Skeleton* skeleton_ = nullptr;
    float time_ = 0.0F;
    bool playing_ = false;
    AnimationPlaybackMode playbackMode_ = AnimationPlaybackMode::Loop;
};

}
