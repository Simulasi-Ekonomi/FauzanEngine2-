#pragma once

#include <cstdint>
#include <vector>

#include "SkeletalPoseClip.h"

namespace NeoEngine {

enum class SkeletalPosePlaybackMode : uint8_t { Clamp, Loop };
enum class SkeletalPosePlayerError : uint8_t { None, NoClip, InvalidSpeed, InvalidDelta, LoopUnavailable, SampleFailed };

class SkeletalPosePlayer {
public:
    static constexpr float kMaxSpeed = 100.0F;
    static constexpr float kMaxDeltaSeconds = 1.0F;

    // Copies a complete clip snapshot; caller may destroy or change the source after a successful bind.
    [[nodiscard]] bool Bind(const SkeletalPoseClip& clip, SkeletalPosePlaybackMode mode);
    [[nodiscard]] bool SetSpeed(float speed);
    void SetPaused(bool paused) { paused_ = paused; }
    // Samples after a bounded caller-provided time step, replacing both time state and output only after success.
    [[nodiscard]] bool Advance(float deltaSeconds, std::vector<Mat4>& output);
    [[nodiscard]] float Time() const { return time_; }
    [[nodiscard]] float Speed() const { return speed_; }
    [[nodiscard]] bool IsPaused() const { return paused_; }
    [[nodiscard]] SkeletalPosePlayerError LastError() const { return lastError_; }

private:
    SkeletalPoseClip clip_{};
    bool hasClip_ = false;
    SkeletalPosePlaybackMode mode_ = SkeletalPosePlaybackMode::Clamp;
    float time_ = 0.0F;
    float speed_ = 1.0F;
    bool paused_ = false;
    SkeletalPosePlayerError lastError_ = SkeletalPosePlayerError::NoClip;
};

} // namespace NeoEngine
