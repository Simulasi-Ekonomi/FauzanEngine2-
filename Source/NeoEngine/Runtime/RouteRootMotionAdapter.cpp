#include "RouteRootMotionAdapter.h"

#include <cmath>
#include <utility>

namespace NeoEngine {

bool RouteRootMotionAdapter::Advance(const float deltaSeconds, GridRouteFollower& follower, SkeletalAnimationController& controller, SceneWorld& world, const SceneEntity entity, const GridNavigation& navigation, MovementAuthorityGate& authority, std::vector<Mat4>& palette) {
    GridRouteFollower candidateFollower = follower;
    SkeletalAnimationController candidateController = controller;
    SceneWorld candidateWorld = world;
    MovementAuthorityGate candidateAuthority = authority;
    std::vector<Mat4> candidatePalette = palette;
    RouteIntent intent{};
    if (!candidateFollower.PeekIntent(candidateWorld, entity, navigation, intent)) { lastError_ = RouteRootMotionAdapterError::IntentFailed; return false; }
    const Transform3* before = candidateWorld.GetLocalTransform(entity);
    if (before == nullptr) { lastError_ = RouteRootMotionAdapterError::IntentFailed; return false; }
    const Transform3 beforeTransform = *before;
    if (!candidateController.AdvanceApplyRootMotionGuarded(deltaSeconds, candidateWorld, entity, candidatePalette, candidateAuthority)) {
        lastError_ = candidateController.LastError() == SkeletalAnimationControllerError::AuthorityConflict ? RouteRootMotionAdapterError::AuthorityConflict : RouteRootMotionAdapterError::RootMotionFailed;
        return false;
    }
    const Transform3* after = candidateWorld.GetLocalTransform(entity);
    if (after == nullptr) { lastError_ = RouteRootMotionAdapterError::RootMotionFailed; return false; }
    constexpr float epsilon = 0.0001F;
    const float deltaX = after->x - beforeTransform.x, deltaY = after->y - beforeTransform.y, deltaZ = after->z - beforeTransform.z;
    const float towardX = intent.targetX - beforeTransform.x, towardZ = intent.targetZ - beforeTransform.z;
    if (!std::isfinite(deltaX) || !std::isfinite(deltaY) || !std::isfinite(deltaZ) || !std::isfinite(towardX) || !std::isfinite(towardZ) || std::fabs(deltaY) > epsilon) { lastError_ = RouteRootMotionAdapterError::DirectionMismatch; return false; }
    const bool horizontal = intent.from.z == intent.target.z;
    const bool aligned = horizontal ? (std::fabs(deltaZ) <= epsilon && deltaX * towardX >= -epsilon) : (std::fabs(deltaX) <= epsilon && deltaZ * towardZ >= -epsilon);
    if (!aligned) { lastError_ = RouteRootMotionAdapterError::DirectionMismatch; return false; }
    if (!candidateFollower.CommitIntent(candidateWorld, navigation, {intent, true})) { lastError_ = RouteRootMotionAdapterError::CommitFailed; return false; }
    follower = std::move(candidateFollower);
    controller = std::move(candidateController);
    world = std::move(candidateWorld);
    authority = std::move(candidateAuthority);
    palette.swap(candidatePalette);
    lastError_ = RouteRootMotionAdapterError::None;
    return true;
}

} // namespace NeoEngine
