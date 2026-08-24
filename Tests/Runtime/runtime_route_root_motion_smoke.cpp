#include "Runtime/NeoRuntime.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace {
bool Near(const float left, const float right) { return std::fabs(left - right) < 0.0001F; }
NeoEngine::Mat4 Translation(const float x, const float y, const float z) { NeoEngine::Mat4 matrix{}; matrix.m[0] = matrix.m[5] = matrix.m[10] = matrix.m[15] = 1.0F; matrix.m[12] = x; matrix.m[13] = y; matrix.m[14] = z; return matrix; }
NeoEngine::SkeletalPoseKeyframe Key(const float time, const float x, const float y, const float z) { NeoEngine::SkeletalPoseKeyframe key{}; key.time = time; key.translation = {x, y, z}; return key; }
bool ConfigureSkeletalRoute(NeoEngine::RuntimeConfig& config, const std::vector<NeoEngine::GridCell>& route, const NeoEngine::SkeletalRouteDirection direction) {
    using namespace NeoEngine;
    Bone root{"root", -1}; root.localBindPose = Translation(0.0F, 0.0F, 0.0F);
    config.enableSkeletalRouteMotion = true; config.skeletalRouteDirection = direction; config.routeMotionNavigationSide = 4U; config.routeMotionRoute = route;
    return config.skeletalRouteSkeleton.TryAddBone(root) && config.skeletalRouteClip.Configure(1U) && config.skeletalRouteClip.SetTrack(0U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F, 0.0F, 0.0F), Key(1.0F, 1.0F, 0.0F, 0.0F)});
}
}

int main() {
    using namespace NeoEngine;
    NeoRuntime invalidSnapshot; RuntimeConfig invalidSnapshotConfig{}; invalidSnapshotConfig.enableSkeletalRouteMotion = true; invalidSnapshotConfig.routeMotionNavigationSide = 4U; invalidSnapshotConfig.routeMotionRoute = {{0U, 0U}, {1U, 0U}}; if (invalidSnapshot.Initialize(invalidSnapshotConfig) || invalidSnapshot.LastError() != RuntimeError::InvalidConfiguration) return 1;
    NeoRuntime loop; RuntimeConfig loopConfig{}; if (!ConfigureSkeletalRoute(loopConfig, {{0U, 0U}, {1U, 0U}}, SkeletalRouteDirection::PositiveX)) return 1; loopConfig.skeletalRoutePlaybackMode = SkeletalPosePlaybackMode::Loop; if (loop.Initialize(loopConfig) || loop.LastError() != RuntimeError::InvalidConfiguration) return 1;
    NeoRuntime dual; RuntimeConfig dualConfig{}; dualConfig.enableRouteMotion = true; if (!ConfigureSkeletalRoute(dualConfig, {{0U, 0U}, {1U, 0U}}, SkeletalRouteDirection::PositiveX) || dual.Initialize(dualConfig) || dual.LastError() != RuntimeError::InvalidConfiguration) return 1;
    NeoRuntime bent; RuntimeConfig bentConfig{}; if (!ConfigureSkeletalRoute(bentConfig, {{0U, 0U}, {1U, 0U}, {1U, 1U}}, SkeletalRouteDirection::PositiveX) || bent.Initialize(bentConfig) || bent.LastError() != RuntimeError::InvalidConfiguration) return 1;
    NeoRuntime runtime; RuntimeConfig config{}; if (!ConfigureSkeletalRoute(config, {{0U, 0U}, {1U, 0U}}, SkeletalRouteDirection::PositiveX) || !runtime.Initialize(config) || runtime.RouteMotionEntity() == nullptr || runtime.SkeletalRouteMotionController() == nullptr || runtime.RouteNavigation() == nullptr || runtime.ReplanRouteMotion() || runtime.LastError() != RuntimeError::RouteReplanFailed) return 1;
    const SceneEntity entity = *runtime.RouteMotionEntity(); if (!runtime.Tick() || runtime.Scene()->GetLocalTransform(entity) == nullptr || !Near(runtime.Scene()->GetLocalTransform(entity)->x, 1.0F / 60.0F) || !Near(runtime.SkeletalRouteMotionController()->Time(), 1.0F / 60.0F) || runtime.MotionAuthority()->Acquire(entity, MovementAuthority::KinematicRoute) || runtime.MotionAuthority()->LastError() != MovementAuthorityError::Conflict) return 1;
    const float pausedX = runtime.Scene()->GetLocalTransform(entity)->x; const float pausedTime = runtime.SkeletalRouteMotionController()->Time(); if (!runtime.SetPaused(true) || !runtime.Tick() || !Near(runtime.Scene()->GetLocalTransform(entity)->x, pausedX) || !Near(runtime.SkeletalRouteMotionController()->Time(), pausedTime) || runtime.MotionAuthority()->Acquire(entity, MovementAuthority::KinematicRoute) || runtime.MotionAuthority()->LastError() != MovementAuthorityError::Conflict) return 1;
    if (!runtime.SetPaused(false)) return 1;
    for (int index = 0; index < 59; ++index) if (!runtime.Tick()) return 1;
    if (runtime.Scene()->GetLocalTransform(entity) == nullptr || !Near(runtime.Scene()->GetLocalTransform(entity)->x, 1.0F) || !Near(runtime.SkeletalRouteMotionController()->Time(), 1.0F)) return 1;
    if (!runtime.Tick() || !Near(runtime.Scene()->GetLocalTransform(entity)->x, 1.0F) || !Near(runtime.SkeletalRouteMotionController()->Time(), 1.0F) || !runtime.MotionAuthority()->Acquire(entity, MovementAuthority::KinematicRoute)) return 1;
    if (!runtime.Shutdown() || runtime.SkeletalRouteMotionController() != nullptr || runtime.RouteMotionEntity() != nullptr) return 1;
    NeoRuntime mismatch; RuntimeConfig mismatchConfig{}; if (!ConfigureSkeletalRoute(mismatchConfig, {{0U, 0U}, {0U, 1U}}, SkeletalRouteDirection::PositiveZ) || !mismatch.Initialize(mismatchConfig) || mismatch.RouteMotionEntity() == nullptr || mismatch.SkeletalRouteMotionController() == nullptr) return 1;
    const SceneEntity mismatchEntity = *mismatch.RouteMotionEntity(); if (mismatch.Tick() || mismatch.LastError() != RuntimeError::RouteMotionFailed || mismatch.Scene()->GetLocalTransform(mismatchEntity) == nullptr || !Near(mismatch.Scene()->GetLocalTransform(mismatchEntity)->z, 0.0F) || !Near(mismatch.SkeletalRouteMotionController()->Time(), 0.0F)) return 1;
    NeoRuntime blocked; RuntimeConfig blockedConfig{}; if (!ConfigureSkeletalRoute(blockedConfig, {{0U, 0U}, {1U, 0U}}, SkeletalRouteDirection::PositiveX) || !blocked.Initialize(blockedConfig) || blocked.RouteNavigation() == nullptr || blocked.RouteMotionEntity() == nullptr || blocked.SkeletalRouteMotionController() == nullptr || !blocked.RouteNavigation()->SetBlocked({1U, 0U}, true)) return 1;
    const SceneEntity blockedEntity = *blocked.RouteMotionEntity(); if (blocked.Tick() || blocked.LastError() != RuntimeError::RouteMotionFailed || blocked.Scene()->GetLocalTransform(blockedEntity) == nullptr || !Near(blocked.Scene()->GetLocalTransform(blockedEntity)->x, 0.0F) || !Near(blocked.SkeletalRouteMotionController()->Time(), 0.0F)) return 1;
    std::printf("RUNTIME_ROUTE_ROOT_MOTION_SMOKE_OK config=1 invalidSnapshot=1 loopRejected=1 partial=1 pause=1 goal=1 authority=1 replan=1 shutdown=1 mismatch=1 blocked=1\n");
    return 0;
}
