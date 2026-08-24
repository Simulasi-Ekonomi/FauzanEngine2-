#pragma once

#include <cstdint>
#include <vector>

#include "Skinning.h"
#include "SkeletalPosePlayer.h"
#include "Skeleton.h"
#include "Runtime/SceneWorld.h"
#include "Runtime/MovementAuthority.h"

namespace NeoEngine {

struct RootMotionDelta { float x = 0.0F; float y = 0.0F; float z = 0.0F; };

enum class SkeletalAnimationControllerError : uint8_t { None, NotInitialized, ClipSkeletonMismatch, SkeletonInvalid, PlayerBindFailed, PlayerAdvanceFailed, PaletteFailed, SkinningFailed, RootMotionLoopUnsupported, RootMotionWrapExceeded, RootMotionInvalid, SceneApplyFailed, AuthorityConflict };

class SkeletalAnimationController {
public:
    // Copies and validates both skeleton and clip before replacing an existing controller configuration.
    [[nodiscard]] bool Initialize(const Skeleton& skeleton, const SkeletalPoseClip& clip, SkeletalPosePlaybackMode mode);
    [[nodiscard]] bool SetSpeed(float speed);
    void SetPaused(bool paused) { player_.SetPaused(paused); }
    // Advances a player snapshot then derives a candidate skinning palette before committing time and caller output.
    [[nodiscard]] bool Advance(float deltaSeconds, std::vector<Mat4>& palette);
    // Advances, derives palette, and skins separate position/normal candidates before committing player time and both buffers.
    [[nodiscard]] bool AdvanceAndSkin(float deltaSeconds, std::vector<float>& positions, std::vector<float>& normals, const std::vector<VertexWeight>& weights);
    // Extracts root local-translation delta while advancing the palette atomically; loop wraps are bounded.
    [[nodiscard]] bool AdvanceWithRootMotion(float deltaSeconds, std::vector<Mat4>& palette, RootMotionDelta& rootMotion);
    // Applies clamp root-motion delta to a SceneWorld local transform through a candidate world snapshot.
    [[nodiscard]] bool AdvanceApplyRootMotion(float deltaSeconds, SceneWorld& world, SceneEntity entity, std::vector<Mat4>& palette);
    [[nodiscard]] bool AdvanceApplyRootMotionGuarded(float deltaSeconds, SceneWorld& world, SceneEntity entity, std::vector<Mat4>& palette, MovementAuthorityGate& authority);
    [[nodiscard]] bool IsInitialized() const { return initialized_; }
    [[nodiscard]] float Time() const { return player_.Time(); }
    [[nodiscard]] SkeletalAnimationControllerError LastError() const { return lastError_; }

private:
    Skeleton skeleton_{};
    SkeletalPosePlayer player_{};
    SkeletalPosePlaybackMode mode_ = SkeletalPosePlaybackMode::Clamp;
    RootMotionDelta rootStartTranslation_{};
    RootMotionDelta rootEndTranslation_{};
    RootMotionDelta rootTranslation_{};
    float rootCycleTime_ = 0.0F;
    float rootLoopDuration_ = 0.0F;
    bool initialized_ = false;
    SkeletalAnimationControllerError lastError_ = SkeletalAnimationControllerError::NotInitialized;
};

} // namespace NeoEngine
