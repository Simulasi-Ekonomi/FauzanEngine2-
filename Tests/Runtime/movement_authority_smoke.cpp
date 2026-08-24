#include <cmath>
#include <cstdio>
#include <vector>

#include "Animation/SkeletalAnimationController.h"
#include "Animation/SkeletalPoseClip.h"
#include "Animation/Skeleton.h"
#include "Runtime/GridRouteFollower.h"
#include "Runtime/MovementAuthority.h"

int main() {
    using namespace NeoEngine;
    SceneWorld world; SceneEntity entity{}; if (!world.Create(entity) || !world.SetTransform(entity, {0,0,0,0,0,0,1,1,1})) return 1;
    KinematicMotionController kinematic; if (!kinematic.Initialize({2.0F, 0.25F})) return 1;
    GridNavigation navigation; if (!navigation.Initialize(4U)) return 1;
    GridRouteFollower follower; const GridCell route[]{{0,0},{1,0}}; if (!follower.SetRoute(route)) return 1;
    Bone root{"root", -1}; Skeleton skeleton; if (!skeleton.TryAddBone(root)) return 1;
    SkeletalPoseClip clip; if (!clip.Configure(1U) || !clip.SetTrack(0U, {{0.0F,{0,0,0},{0,0,0,1},{1,1,1}}, {1.0F,{1,0,0},{0,0,0,1},{1,1,1}}})) return 1;
    SkeletalAnimationController skeletal; if (!skeletal.Initialize(skeleton, clip, SkeletalPosePlaybackMode::Clamp)) return 1;
    MovementAuthorityGate authority; if (authority.Acquire(SceneEntity{}, MovementAuthority::KinematicRoute) || authority.LastError() != MovementAuthorityError::InvalidEntity) return 1;
    authority.BeginFrame();
    if (!follower.StepGuarded(world, entity, kinematic, navigation, 0.25F, authority) || std::fabs(world.GetLocalTransform(entity)->x - 0.5F) > 0.0001F) return 1;
    std::vector<Mat4> palette;
    if (skeletal.AdvanceApplyRootMotionGuarded(0.5F, world, entity, palette, authority) || skeletal.LastError() != SkeletalAnimationControllerError::AuthorityConflict || std::fabs(world.GetLocalTransform(entity)->x - 0.5F) > 0.0001F || std::fabs(skeletal.Time()) > 0.0001F) return 1;
    authority.BeginFrame();
    if (!skeletal.AdvanceApplyRootMotionGuarded(0.5F, world, entity, palette, authority) || std::fabs(world.GetLocalTransform(entity)->x - 1.0F) > 0.0001F || std::fabs(skeletal.Time() - 0.5F) > 0.0001F) return 1;
    if (!authority.Acquire(entity, MovementAuthority::SkeletalRoot) || authority.Acquire(entity, MovementAuthority::KinematicRoute) || authority.LastError() != MovementAuthorityError::Conflict) return 1;
    std::printf("MOVEMENT_AUTHORITY_SMOKE_OK route=1 skeletal=1 conflict=1 reset=1 atomic=1 x=%.1f\n", world.GetLocalTransform(entity)->x);
}
