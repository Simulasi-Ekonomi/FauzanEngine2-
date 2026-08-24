#include "Animation/SkeletalPoseClip.h"
#include "Animation/Skeleton.h"
#include "Animation/Skinning.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace {
bool Near(const float left, const float right) { return std::fabs(left - right) < 0.0001F; }
NeoEngine::SkeletalPoseKeyframe Key(const float time, const float tx, const float ty, const float tz, const NeoEngine::PoseQuat rotation = {}) {
    NeoEngine::SkeletalPoseKeyframe key{}; key.time = time; key.translation = {tx, ty, tz}; key.rotation = rotation; return key;
}
}

int main() {
    using namespace NeoEngine;
    SkeletalPoseClip clip;
    if (!clip.Configure(3U)) return 1;
    if (!clip.SetTrack(0U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F, 0.0F, 0.0F), Key(1.0F, 2.0F, 0.0F, 0.0F)})) return 1;
    if (!clip.SetTrack(1U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F, 2.0F, 0.0F)})) return 1;
    if (!clip.SetTrack(2U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F, 0.0F, 3.0F), Key(1.0F, 0.0F, 0.0F, 3.0F, PoseQuat{0.0F, 0.0F, 1.0F, 0.0F})})) return 1;
    std::vector<Mat4> localPose;
    if (!clip.Sample(0.5F, localPose) || localPose.size() != 3U || !Near(localPose[0].m[12], 1.0F) || !Near(localPose[1].m[13], 2.0F) || !Near(localPose[2].m[14], 3.0F) || !Near(localPose[2].m[0], 0.0F) || !Near(localPose[2].m[1], 1.0F) || !Near(localPose[2].m[4], -1.0F) || !Near(localPose[2].m[5], 0.0F)) return 1;
    if (!Near(clip.Duration(), 1.0F) || !clip.SampleLooped(1.5F, localPose) || !Near(localPose[0].m[12], 1.0F) || !Near(localPose[2].m[1], 1.0F)) return 1;
    SkeletalPoseClip blendTarget; if (!blendTarget.Configure(3U) || !blendTarget.SetTrack(0U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 4.0F, 0.0F, 0.0F)}) || !blendTarget.SetTrack(1U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F, 2.0F, 0.0F)}) || !blendTarget.SetTrack(2U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F, 0.0F, 3.0F)})) return 1;
    std::vector<Mat4> blended;
    if (!clip.SampleBlended(blendTarget, 0.5F, 0.5F, 0.5F, blended) || blended.size() != 3U || !Near(blended[0].m[12], 2.5F) || !Near(blended[2].m[0], 0.7071068F) || !Near(blended[2].m[1], 0.7071068F) || !Near(blended[2].m[4], -0.7071068F) || !Near(blended[2].m[5], 0.7071068F)) return 1;
    const std::vector<Mat4> stableBlend = blended;
    if (clip.SampleBlended(blendTarget, 0.5F, 0.5F, 1.1F, blended) || clip.LastError() != SkeletalPoseError::InvalidBlendFactor || blended.size() != stableBlend.size() || !Near(blended[0].m[12], stableBlend[0].m[12])) return 1;
    SkeletalPoseClip incompatible; if (!incompatible.Configure(2U) || clip.SampleBlended(incompatible, 0.5F, 0.5F, 0.5F, blended) || clip.LastError() != SkeletalPoseError::BlendIncompatible || blended.size() != stableBlend.size() || !Near(blended[2].m[5], stableBlend[2].m[5])) return 1;
    std::vector<Mat4> bindPose; if (!clip.Sample(0.0F, bindPose)) return 1;
    Skeleton skeleton;
    Bone root{"root", -1}; root.localBindPose = bindPose[0U];
    Bone spine{"spine", 0}; spine.localBindPose = bindPose[1U];
    Bone head{"head", 1}; head.localBindPose = bindPose[2U];
    if (!skeleton.TryAddBone(root) || !skeleton.TryAddBone(spine) || !skeleton.TryAddBone(head) || !skeleton.DeriveInverseBindPose()) return 1;
    std::vector<Mat4> palette; if (!skeleton.EvaluateSkinningPalette(localPose, palette) || palette.size() != 3U) return 1;
    std::vector<float> skinnedPosition{0.0F, 2.0F, 3.0F}; VertexWeight headWeight{}; headWeight.boneIDs[0] = 2; headWeight.weights[0] = 1.0F; SkinningError skinningError = SkinningError::None;
    if (!Skinning::ApplySkinning(skinnedPosition, std::vector<VertexWeight>{headWeight}, palette, &skinningError) || skinningError != SkinningError::None || !Near(skinnedPosition[0], 1.0F) || !Near(skinnedPosition[1], 2.0F) || !Near(skinnedPosition[2], 3.0F)) return 1;
    std::vector<float> normalPathPosition{0.0F, 2.0F, 3.0F}; std::vector<float> normalPathNormal{1.0F, 0.0F, 0.0F};
    if (!Skinning::ApplySkinningWithNormals(normalPathPosition, normalPathNormal, std::vector<VertexWeight>{headWeight}, palette, &skinningError) || skinningError != SkinningError::None || !Near(normalPathPosition[0], 1.0F) || !Near(normalPathPosition[1], 2.0F) || !Near(normalPathPosition[2], 3.0F) || !Near(normalPathNormal[0], 0.0F) || !Near(normalPathNormal[1], 1.0F) || !Near(normalPathNormal[2], 0.0F)) return 1;
    const std::vector<float> stableNormalPathPosition = normalPathPosition; const std::vector<float> stableNormalPathNormal = normalPathNormal; std::vector<float> invalidNormal{0.0F, 0.0F, 0.0F};
    if (Skinning::ApplySkinningWithNormals(normalPathPosition, invalidNormal, std::vector<VertexWeight>{headWeight}, palette, &skinningError) || skinningError != SkinningError::InvalidNormal || normalPathPosition != stableNormalPathPosition || invalidNormal != std::vector<float>{0.0F, 0.0F, 0.0F} || normalPathNormal != stableNormalPathNormal) return 1;
    const std::vector<Mat4> stable = localPose;
    if (clip.Sample(-1.0F, localPose) || clip.LastError() != SkeletalPoseError::InvalidSampleTime || localPose.size() != stable.size() || !Near(localPose[0].m[12], stable[0].m[12])) return 1;
    if (clip.SetTrack(1U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F, 0.0F, 0.0F)}) || clip.LastError() != SkeletalPoseError::DuplicateTrack) return 1;
    SkeletalPoseClip incomplete; if (!incomplete.Configure(2U) || !incomplete.SetTrack(0U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F, 0.0F, 0.0F)}) || incomplete.Sample(0.0F, localPose) || incomplete.LastError() != SkeletalPoseError::IncompleteClip || localPose.size() != stable.size() || !Near(localPose[2].m[14], stable[2].m[14])) return 1;
    SkeletalPoseClip staticClip; if (!staticClip.Configure(1U) || !staticClip.SetTrack(0U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F, 0.0F, 0.0F)}) || staticClip.SampleLooped(0.5F, localPose) || staticClip.LastError() != SkeletalPoseError::LoopUnavailable || localPose.size() != stable.size() || !Near(localPose[0].m[12], stable[0].m[12])) return 1;
    SkeletalPoseClip invalid; if (!invalid.Configure(1U) || invalid.SetTrack(0U, std::vector<SkeletalPoseKeyframe>{Key(0.1F, 0.0F, 0.0F, 0.0F)}) || invalid.LastError() != SkeletalPoseError::InvalidKey) return 1;
    std::printf("SKELETAL_POSE_CLIP_SMOKE_OK bones=3 keys=2 interpolation=1 blend=1 loop=1 quaternion=1 palette=1 skinningPath=1 normalPath=1 atomic=1 bounded=64\n");
    return 0;
}
