#pragma once

#include "Animation/SkeletalAnimationController.h"
#include "GridRouteFollower.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {

enum class RouteRootMotionAdapterError : uint8_t { None, IntentFailed, RootMotionFailed, DirectionMismatch, CommitFailed, AuthorityConflict };

class RouteRootMotionAdapter {
public:
    // The skeletal controller is the only transform writer. Route intent and cursor commit never invoke kinematic motion.
    [[nodiscard]] bool Advance(float deltaSeconds, GridRouteFollower& follower, SkeletalAnimationController& controller, SceneWorld& world, SceneEntity entity, const GridNavigation& navigation, MovementAuthorityGate& authority, std::vector<Mat4>& palette);
    [[nodiscard]] RouteRootMotionAdapterError LastError() const { return lastError_; }
private:
    RouteRootMotionAdapterError lastError_ = RouteRootMotionAdapterError::None;
};

} // namespace NeoEngine
