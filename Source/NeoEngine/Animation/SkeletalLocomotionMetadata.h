#pragma once

#include "SkeletalPoseClip.h"

#include <cstdint>
#include <span>
#include <vector>

namespace NeoEngine {
enum class SkeletalLocomotionDirection : uint8_t { PositiveX, NegativeX, PositiveZ, NegativeZ };
enum class SkeletalLocomotionMetadataError : uint8_t { None, ClipInvalid, DurationInvalid, RootStartInvalid, RootTerminalInvalid };
class SkeletalLocomotionMetadata {
public:
    // Validates a complete clamp clip with zero root start and an exact one-cell cardinal root terminal.
    [[nodiscard]] static bool ValidateCardinalOneCell(const SkeletalPoseClip& clip, SkeletalLocomotionDirection direction, SkeletalLocomotionMetadataError& error);
};
struct SkeletalLocomotionClipSlot { SkeletalLocomotionDirection direction{}; SkeletalPoseClip clip{}; };
enum class SkeletalLocomotionRegistryError : uint8_t { None, Empty, TooMany, DuplicateDirection, InvalidClip };
class SkeletalLocomotionRegistry {
public:
    [[nodiscard]] bool Configure(std::span<const SkeletalLocomotionClipSlot> slots);
    [[nodiscard]] const SkeletalPoseClip* Find(SkeletalLocomotionDirection direction) const;
    [[nodiscard]] SkeletalLocomotionRegistryError LastError() const { return lastError_; }
private:
    std::vector<SkeletalLocomotionClipSlot> slots_;
    SkeletalLocomotionRegistryError lastError_ = SkeletalLocomotionRegistryError::Empty;
};
} // namespace NeoEngine
