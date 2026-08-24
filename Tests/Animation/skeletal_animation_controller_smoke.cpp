#include "Animation/SkeletalAnimationController.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace {
bool Near(const float left, const float right) { return std::fabs(left - right) < 0.0001F; }
NeoEngine::Mat4 Translation(const float x, const float y, const float z) { NeoEngine::Mat4 matrix{}; matrix.m[0] = matrix.m[5] = matrix.m[10] = matrix.m[15] = 1.0F; matrix.m[12] = x; matrix.m[13] = y; matrix.m[14] = z; return matrix; }
NeoEngine::SkeletalPoseKeyframe Key(const float time, const float x, const float y, const float z) { NeoEngine::SkeletalPoseKeyframe key{}; key.time = time; key.translation = {x, y, z}; return key; }
}

int main() {
    using namespace NeoEngine;
    Skeleton skeleton; Bone root{"root", -1}; root.localBindPose = Translation(0.0F, 0.0F, 0.0F); Bone spine{"spine", 0}; spine.localBindPose = Translation(0.0F, 2.0F, 0.0F); Bone head{"head", 1}; head.localBindPose = Translation(0.0F, 0.0F, 3.0F);
    if (!skeleton.TryAddBone(root) || !skeleton.TryAddBone(spine) || !skeleton.TryAddBone(head)) return 1;
    SkeletalPoseClip clip; if (!clip.Configure(3U) || !clip.SetTrack(0U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F, 0.0F, 0.0F), Key(1.0F, 2.0F, 0.0F, 0.0F)}) || !clip.SetTrack(1U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F, 2.0F, 0.0F)}) || !clip.SetTrack(2U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F, 0.0F, 3.0F)})) return 1;
    SkeletalAnimationController controller; std::vector<Mat4> palette;
    if (!controller.Initialize(skeleton, clip, SkeletalPosePlaybackMode::Clamp) || !controller.Advance(0.5F, palette) || !Near(controller.Time(), 0.5F) || palette.size() != 3U || !Near(palette[2].m[12], 1.0F) || !Near(palette[2].m[13], 0.0F) || !Near(palette[2].m[14], 0.0F)) return 1;
    controller.SetPaused(true); if (!controller.Advance(0.25F, palette) || !Near(controller.Time(), 0.5F) || !Near(palette[2].m[12], 1.0F)) return 1;
    controller.SetPaused(false); if (!controller.SetSpeed(2.0F) || !controller.Advance(0.25F, palette) || !Near(controller.Time(), 1.0F) || !Near(palette[2].m[12], 2.0F)) return 1;
    const std::vector<Mat4> stable = palette; const float stableTime = controller.Time();
    if (controller.Advance(-0.1F, palette) || controller.LastError() != SkeletalAnimationControllerError::PlayerAdvanceFailed || !Near(controller.Time(), stableTime) || palette.size() != stable.size() || !Near(palette[2].m[12], stable[2].m[12])) return 1;
    SkeletalPoseClip shortClip; if (!shortClip.Configure(2U) || controller.Initialize(skeleton, shortClip, SkeletalPosePlaybackMode::Clamp) || controller.LastError() != SkeletalAnimationControllerError::ClipSkeletonMismatch || !controller.Advance(0.0F, palette) || !Near(controller.Time(), stableTime) || !Near(palette[2].m[12], stable[2].m[12])) return 1;
    SkeletalAnimationController skinController; std::vector<float> positions{0.0F, 2.0F, 3.0F}; std::vector<float> normals{1.0F, 0.0F, 0.0F}; VertexWeight headWeight{}; headWeight.boneIDs[0] = 2; headWeight.weights[0] = 1.0F;
    if (!skinController.Initialize(skeleton, clip, SkeletalPosePlaybackMode::Clamp) || !skinController.AdvanceAndSkin(0.5F, positions, normals, std::vector<VertexWeight>{headWeight}) || !Near(skinController.Time(), 0.5F) || !Near(positions[0], 1.0F) || !Near(positions[1], 2.0F) || !Near(positions[2], 3.0F) || !Near(normals[0], 1.0F) || !Near(normals[1], 0.0F) || !Near(normals[2], 0.0F)) return 1;
    const std::vector<float> stablePositions = positions; const std::vector<float> stableNormals = normals; const float stableSkinTime = skinController.Time(); VertexWeight invalidWeight = headWeight; invalidWeight.boneIDs[0] = 3;
    if (skinController.AdvanceAndSkin(0.25F, positions, normals, std::vector<VertexWeight>{invalidWeight}) || skinController.LastError() != SkeletalAnimationControllerError::SkinningFailed || !Near(skinController.Time(), stableSkinTime) || positions != stablePositions || normals != stableNormals) return 1;
    SkeletalAnimationController rootController; RootMotionDelta rootMotion{};
    if (!rootController.Initialize(skeleton, clip, SkeletalPosePlaybackMode::Clamp) || !rootController.AdvanceWithRootMotion(0.5F, palette, rootMotion) || !Near(rootMotion.x, 1.0F) || !Near(rootMotion.y, 0.0F) || !Near(rootMotion.z, 0.0F) || !Near(rootController.Time(), 0.5F)) return 1;
    rootController.SetPaused(true); if (!rootController.AdvanceWithRootMotion(0.25F, palette, rootMotion) || !Near(rootMotion.x, 0.0F) || !Near(rootController.Time(), 0.5F)) return 1;
    rootController.SetPaused(false); if (!rootController.SetSpeed(2.0F) || !rootController.AdvanceWithRootMotion(0.25F, palette, rootMotion) || !Near(rootMotion.x, 1.0F) || !Near(rootController.Time(), 1.0F)) return 1;
    if (!rootController.AdvanceWithRootMotion(0.25F, palette, rootMotion) || !Near(rootMotion.x, 0.0F) || !Near(rootController.Time(), 1.0F)) return 1;
    const RootMotionDelta stableRootMotion = rootMotion; const float stableRootTime = rootController.Time();
    if (rootController.AdvanceWithRootMotion(-0.1F, palette, rootMotion) || rootController.LastError() != SkeletalAnimationControllerError::PlayerAdvanceFailed || !Near(rootController.Time(), stableRootTime) || !Near(rootMotion.x, stableRootMotion.x)) return 1;
    SkeletalAnimationController loopController; if (!loopController.Initialize(skeleton, clip, SkeletalPosePlaybackMode::Loop) || !loopController.SetSpeed(2.0F) || !loopController.AdvanceWithRootMotion(0.75F, palette, rootMotion) || !Near(loopController.Time(), 0.5F) || !Near(rootMotion.x, 3.0F)) return 1;
    if (!loopController.SetSpeed(4.0F) || !loopController.AdvanceWithRootMotion(0.75F, palette, rootMotion) || !Near(loopController.Time(), 0.5F) || !Near(rootMotion.x, 6.0F)) return 1;
    SceneWorld world; SceneEntity actor{}; Transform3 actorTransform{}; actorTransform.x = 10.0F;
    if (!world.Create(actor) || !world.SetTransform(actor, actorTransform)) return 1;
    SkeletalAnimationController entityController; if (!entityController.Initialize(skeleton, clip, SkeletalPosePlaybackMode::Clamp) || !entityController.AdvanceApplyRootMotion(0.5F, world, actor, palette) || !Near(entityController.Time(), 0.5F) || world.GetLocalTransform(actor) == nullptr || !Near(world.GetLocalTransform(actor)->x, 11.0F) || !Near(palette[2].m[12], 0.0F)) return 1;
    const float stableEntityTime = entityController.Time();
    if (entityController.AdvanceApplyRootMotion(0.25F, world, SceneEntity{}, palette) || entityController.LastError() != SkeletalAnimationControllerError::SceneApplyFailed || !Near(entityController.Time(), stableEntityTime) || !Near(world.GetLocalTransform(actor)->x, 11.0F)) return 1;
    SceneWorld loopWorld; SceneEntity loopActor{}; if (!loopWorld.Create(loopActor) || !loopWorld.SetTransform(loopActor, actorTransform)) return 1;
    SkeletalAnimationController loopEntityController; if (!loopEntityController.Initialize(skeleton, clip, SkeletalPosePlaybackMode::Loop) || !loopEntityController.SetSpeed(2.0F) || !loopEntityController.AdvanceApplyRootMotion(0.75F, loopWorld, loopActor, palette) || !Near(loopEntityController.Time(), 0.5F) || !Near(loopWorld.GetLocalTransform(loopActor)->x, 13.0F) || !Near(palette[2].m[12], 0.0F)) return 1;
    if (!loopEntityController.SetSpeed(4.0F) || !loopEntityController.AdvanceApplyRootMotion(0.75F, loopWorld, loopActor, palette) || !Near(loopEntityController.Time(), 0.5F) || !Near(loopWorld.GetLocalTransform(loopActor)->x, 19.0F) || !Near(palette[2].m[12], 0.0F)) return 1;
    std::printf("SKELETAL_ANIMATION_CONTROLLER_SMOKE_OK bones=3 hierarchy=1 player=1 palette=1 skinning=1 rootMotion=1 sceneApply=1 loopSceneApply=1 pause=1 atomic=1\n");
    return 0;
}
