#include "Animation/SkeletalLocomotionMetadata.h"

#include <cstdio>
#include <vector>

using namespace NeoEngine;
namespace {
SkeletalPoseKeyframe Key(float time, float x, float y, float z) { SkeletalPoseKeyframe key{}; key.time = time; key.translation = {x, y, z}; return key; }
bool Make(SkeletalPoseClip& clip, std::vector<SkeletalPoseKeyframe> keys) { return clip.Configure(1U) && clip.SetTrack(0U, keys); }
}
int main() {
    SkeletalLocomotionMetadataError error{}; SkeletalPoseClip positiveX;
    if (!Make(positiveX, {Key(0.0F, 0.0F, 0.0F, 0.0F), Key(1.0F, 1.0F, 0.0F, 0.0F)}) || !SkeletalLocomotionMetadata::ValidateCardinalOneCell(positiveX, SkeletalLocomotionDirection::PositiveX, error) || error != SkeletalLocomotionMetadataError::None) return 1;
    if (SkeletalLocomotionMetadata::ValidateCardinalOneCell(positiveX, SkeletalLocomotionDirection::PositiveZ, error) || error != SkeletalLocomotionMetadataError::RootTerminalInvalid) return 1;
    SkeletalPoseClip nonZeroStart; if (!Make(nonZeroStart, {Key(0.0F, 0.1F, 0.0F, 0.0F), Key(1.0F, 1.0F, 0.0F, 0.0F)}) || SkeletalLocomotionMetadata::ValidateCardinalOneCell(nonZeroStart, SkeletalLocomotionDirection::PositiveX, error) || error != SkeletalLocomotionMetadataError::RootStartInvalid) return 1;
    SkeletalPoseClip diagonal; if (!Make(diagonal, {Key(0.0F, 0.0F, 0.0F, 0.0F), Key(1.0F, 1.0F, 0.0F, 1.0F)}) || SkeletalLocomotionMetadata::ValidateCardinalOneCell(diagonal, SkeletalLocomotionDirection::PositiveX, error) || error != SkeletalLocomotionMetadataError::RootTerminalInvalid) return 1;
    SkeletalPoseClip longClip; if (!Make(longClip, {Key(0.0F, 0.0F, 0.0F, 0.0F), Key(2.0F, 1.0F, 0.0F, 0.0F)}) || SkeletalLocomotionMetadata::ValidateCardinalOneCell(longClip, SkeletalLocomotionDirection::PositiveX, error) || error != SkeletalLocomotionMetadataError::DurationInvalid) return 1;
    SkeletalLocomotionRegistry registry; const SkeletalLocomotionClipSlot valid[]{ {SkeletalLocomotionDirection::PositiveX, positiveX} }; if (!registry.Configure(valid) || registry.LastError() != SkeletalLocomotionRegistryError::None || registry.Find(SkeletalLocomotionDirection::PositiveX) == nullptr || registry.Find(SkeletalLocomotionDirection::PositiveZ) != nullptr) return 1;
    const SkeletalLocomotionClipSlot duplicate[]{ {SkeletalLocomotionDirection::PositiveX, positiveX}, {SkeletalLocomotionDirection::PositiveX, positiveX} }; if (registry.Configure(duplicate) || registry.LastError() != SkeletalLocomotionRegistryError::DuplicateDirection || registry.Find(SkeletalLocomotionDirection::PositiveX) == nullptr) return 1;
    std::printf("SKELETAL_LOCOMOTION_METADATA_SMOKE_OK cardinal=1 direction=1 start=1 terminal=1 duration=1 registry=1 duplicate=1\n"); return 0;
}
