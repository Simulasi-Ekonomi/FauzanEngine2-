#include "Runtime/RouteRootMotionAdapter.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace {
bool Near(const float left, const float right) { return std::fabs(left - right) < 0.0001F; }
NeoEngine::Mat4 Translation(const float x, const float y, const float z) { NeoEngine::Mat4 matrix{}; matrix.m[0] = matrix.m[5] = matrix.m[10] = matrix.m[15] = 1.0F; matrix.m[12] = x; matrix.m[13] = y; matrix.m[14] = z; return matrix; }
NeoEngine::SkeletalPoseKeyframe Key(const float time, const float x, const float y, const float z) { NeoEngine::SkeletalPoseKeyframe key{}; key.time = time; key.translation = {x, y, z}; return key; }
bool ConfigureController(NeoEngine::SkeletalAnimationController& controller) {
    NeoEngine::Skeleton skeleton; NeoEngine::Bone root{"root", -1}; root.localBindPose = Translation(0.0F, 0.0F, 0.0F);
    NeoEngine::SkeletalPoseClip clip;
    return skeleton.TryAddBone(root) && clip.Configure(1U) && clip.SetTrack(0U, std::vector<NeoEngine::SkeletalPoseKeyframe>{Key(0.0F, 0.0F, 0.0F, 0.0F), Key(1.0F, 1.0F, 0.0F, 0.0F)}) && controller.Initialize(skeleton, clip, NeoEngine::SkeletalPosePlaybackMode::Clamp);
}
bool ConfigureRoute(NeoEngine::GridRouteFollower& follower, const std::vector<NeoEngine::GridCell>& route) { return follower.SetRoute(route); }
}

int main() {
    using namespace NeoEngine;
    GridNavigation navigation; if (!navigation.Initialize(4U)) return 1;
    SceneWorld world; SceneEntity actor{}; if (!world.Create(actor) || !world.SetTransform(actor, {0.0F, 0.0F, 0.0F, 0, 0, 0, 1, 1, 1})) return 1;
    GridRouteFollower follower; if (!ConfigureRoute(follower, {{0U, 0U}, {1U, 0U}, {1U, 1U}})) return 1;
    SkeletalAnimationController controller; if (!ConfigureController(controller)) return 1;
    MovementAuthorityGate authority; RouteRootMotionAdapter adapter; std::vector<Mat4> palette;
    authority.BeginFrame(); if (!adapter.Advance(0.5F, follower, controller, world, actor, navigation, authority, palette) || !Near(controller.Time(), 0.5F) || world.GetLocalTransform(actor) == nullptr || !Near(world.GetLocalTransform(actor)->x, 0.5F) || follower.ReachedGoal()) return 1;
    if (!adapter.Advance(0.5F, follower, controller, world, actor, navigation, authority, palette) || !Near(controller.Time(), 1.0F) || world.GetLocalTransform(actor) == nullptr || !Near(world.GetLocalTransform(actor)->x, 1.0F) || !Near(world.GetLocalTransform(actor)->z, 0.0F) || follower.ReachedGoal()) return 1;
    SceneWorld mismatchWorld; SceneEntity mismatchActor{}; if (!mismatchWorld.Create(mismatchActor) || !mismatchWorld.SetTransform(mismatchActor, {0.0F, 0.0F, 0.0F, 0, 0, 0, 1, 1, 1})) return 1;
    GridRouteFollower mismatchFollower; SkeletalAnimationController mismatchController; MovementAuthorityGate mismatchAuthority; RouteRootMotionAdapter mismatchAdapter; std::vector<Mat4> mismatchPalette{Translation(9.0F, 0.0F, 0.0F)};
    if (!ConfigureRoute(mismatchFollower, {{0U, 0U}, {0U, 1U}}) || !ConfigureController(mismatchController)) return 1;
    mismatchAuthority.BeginFrame(); if (mismatchAdapter.Advance(0.5F, mismatchFollower, mismatchController, mismatchWorld, mismatchActor, navigation, mismatchAuthority, mismatchPalette) || mismatchAdapter.LastError() != RouteRootMotionAdapterError::DirectionMismatch || !Near(mismatchController.Time(), 0.0F) || mismatchWorld.GetLocalTransform(mismatchActor) == nullptr || !Near(mismatchWorld.GetLocalTransform(mismatchActor)->x, 0.0F) || mismatchPalette.size() != 1U || !Near(mismatchPalette[0].m[12], 9.0F)) return 1;
    RouteIntent mismatchIntent{}; if (!mismatchFollower.PeekIntent(mismatchWorld, mismatchActor, navigation, mismatchIntent) || mismatchIntent.target != GridCell{0U, 1U}) return 1;
    SceneWorld conflictWorld; SceneEntity conflictActor{}; if (!conflictWorld.Create(conflictActor) || !conflictWorld.SetTransform(conflictActor, {0.0F, 0.0F, 0.0F, 0, 0, 0, 1, 1, 1})) return 1;
    GridRouteFollower conflictFollower; SkeletalAnimationController conflictController; MovementAuthorityGate conflictAuthority; RouteRootMotionAdapter conflictAdapter; std::vector<Mat4> conflictPalette;
    if (!ConfigureRoute(conflictFollower, {{0U, 0U}, {1U, 0U}}) || !ConfigureController(conflictController)) return 1;
    conflictAuthority.BeginFrame(); if (!conflictAuthority.Acquire(conflictActor, MovementAuthority::KinematicRoute) || conflictAdapter.Advance(0.5F, conflictFollower, conflictController, conflictWorld, conflictActor, navigation, conflictAuthority, conflictPalette) || conflictAdapter.LastError() != RouteRootMotionAdapterError::AuthorityConflict || !Near(conflictController.Time(), 0.0F) || conflictWorld.GetLocalTransform(conflictActor) == nullptr || !Near(conflictWorld.GetLocalTransform(conflictActor)->x, 0.0F)) return 1;
    if (!navigation.SetBlocked({1U, 1U}, true) || adapter.Advance(0.0F, follower, controller, world, actor, navigation, authority, palette) || adapter.LastError() != RouteRootMotionAdapterError::IntentFailed || world.GetLocalTransform(actor) == nullptr || !Near(world.GetLocalTransform(actor)->x, 1.0F) || !navigation.SetBlocked({1U, 1U}, false)) return 1;
    std::printf("ROUTE_ROOT_MOTION_ADAPTER_SMOKE_OK skeletal_writer=1 partial=1 arrival=1 direction=1 authority=1 blocked=1\n");
    return 0;
}
