#include "SkeletalLocomotionMetadata.h"

#include <cmath>
#include <vector>

namespace NeoEngine {
bool SkeletalLocomotionMetadata::ValidateCardinalOneCell(const SkeletalPoseClip& clip, const SkeletalLocomotionDirection direction, SkeletalLocomotionMetadataError& error) {
    constexpr float epsilon = 0.0001F;
    const float duration = clip.Duration();
    if (clip.BoneCount() == 0U) { error = SkeletalLocomotionMetadataError::ClipInvalid; return false; }
    if (!std::isfinite(duration) || duration <= 0.0F || duration > 1.0F) { error = SkeletalLocomotionMetadataError::DurationInvalid; return false; }
    SkeletalPoseClip candidate = clip;
    std::vector<Mat4> start, terminal;
    if (!candidate.Sample(0.0F, start) || !candidate.Sample(duration, terminal) || start.empty() || terminal.empty()) { error = SkeletalLocomotionMetadataError::ClipInvalid; return false; }
    const Mat4& begin = start.front(); const Mat4& end = terminal.front();
    if (std::fabs(begin.m[12]) > epsilon || std::fabs(begin.m[13]) > epsilon || std::fabs(begin.m[14]) > epsilon) { error = SkeletalLocomotionMetadataError::RootStartInvalid; return false; }
    const float x = end.m[12], y = end.m[13], z = end.m[14];
    const bool valid = std::fabs(y) <= epsilon && ((direction == SkeletalLocomotionDirection::PositiveX && std::fabs(x - 1.0F) <= epsilon && std::fabs(z) <= epsilon) || (direction == SkeletalLocomotionDirection::NegativeX && std::fabs(x + 1.0F) <= epsilon && std::fabs(z) <= epsilon) || (direction == SkeletalLocomotionDirection::PositiveZ && std::fabs(x) <= epsilon && std::fabs(z - 1.0F) <= epsilon) || (direction == SkeletalLocomotionDirection::NegativeZ && std::fabs(x) <= epsilon && std::fabs(z + 1.0F) <= epsilon));
    if (!valid) { error = SkeletalLocomotionMetadataError::RootTerminalInvalid; return false; }
    error = SkeletalLocomotionMetadataError::None; return true;
}
bool SkeletalLocomotionRegistry::Configure(const std::span<const SkeletalLocomotionClipSlot> slots) {
    if (slots.empty()) { lastError_ = SkeletalLocomotionRegistryError::Empty; return false; }
    if (slots.size() > 4U) { lastError_ = SkeletalLocomotionRegistryError::TooMany; return false; }
    std::vector<SkeletalLocomotionClipSlot> candidate(slots.begin(), slots.end());
    for (size_t index = 0; index < candidate.size(); ++index) {
        SkeletalLocomotionMetadataError error{};
        if (!SkeletalLocomotionMetadata::ValidateCardinalOneCell(candidate[index].clip, candidate[index].direction, error)) { lastError_ = SkeletalLocomotionRegistryError::InvalidClip; return false; }
        for (size_t other = 0; other < index; ++other) if (candidate[index].direction == candidate[other].direction) { lastError_ = SkeletalLocomotionRegistryError::DuplicateDirection; return false; }
    }
    slots_ = std::move(candidate); lastError_ = SkeletalLocomotionRegistryError::None; return true;
}
const SkeletalPoseClip* SkeletalLocomotionRegistry::Find(const SkeletalLocomotionDirection direction) const { for (const auto& slot : slots_) if (slot.direction == direction) return &slot.clip; return nullptr; }
} // namespace NeoEngine
