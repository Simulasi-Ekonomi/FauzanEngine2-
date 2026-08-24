#pragma once

#include <cstdint>
#include <vector>

#include "Core/Math/Mat4.h"

namespace NeoEngine {

struct PoseVec3 { float x = 0.0F; float y = 0.0F; float z = 0.0F; };
struct PoseQuat { float x = 0.0F; float y = 0.0F; float z = 0.0F; float w = 1.0F; };
struct SkeletalPoseKeyframe {
    float time = 0.0F;
    PoseVec3 translation{};
    PoseQuat rotation{};
    PoseVec3 scale{1.0F, 1.0F, 1.0F};
};

enum class SkeletalPoseError : uint8_t { None, BoneCountExceeded, NotConfigured, InvalidTrack, DuplicateTrack, InvalidKey, IncompleteClip, InvalidSampleTime, LoopUnavailable, InvalidBlendFactor, BlendIncompatible, InvalidPose };

class SkeletalPoseClip {
public:
    static constexpr size_t kMaxBones = 64U;
    static constexpr size_t kMaxKeysPerTrack = 128U;

    [[nodiscard]] bool Configure(size_t boneCount);
    [[nodiscard]] bool SetTrack(size_t boneIndex, const std::vector<SkeletalPoseKeyframe>& keys);
    // Samples a complete local affine pose without replacing caller output if any track/key is invalid.
    [[nodiscard]] bool Sample(float time, std::vector<Mat4>& output);
    // Wraps positive sample time by the complete clip duration; static clips intentionally reject looping.
    [[nodiscard]] bool SampleLooped(float time, std::vector<Mat4>& output);
    // Samples two compatible clips and blends their TRS components into a candidate local pose.
    [[nodiscard]] bool SampleBlended(const SkeletalPoseClip& other, float time, float otherTime, float blendFactor, std::vector<Mat4>& output);
    [[nodiscard]] float Duration() const;
    [[nodiscard]] size_t BoneCount() const { return tracks_.size(); }
    [[nodiscard]] SkeletalPoseError LastError() const { return lastError_; }

private:
    std::vector<std::vector<SkeletalPoseKeyframe>> tracks_;
    SkeletalPoseError lastError_ = SkeletalPoseError::NotConfigured;
};

} // namespace NeoEngine
