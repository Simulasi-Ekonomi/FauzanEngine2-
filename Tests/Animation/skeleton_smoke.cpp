#include "Animation/Skeleton.h"
#include "Animation/Skinning.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <string>

namespace {
NeoEngine::Mat4 Translation(float x, float y, float z) {
    NeoEngine::Mat4 matrix{};
    matrix.m[0] = matrix.m[5] = matrix.m[10] = matrix.m[15] = 1.0F;
    matrix.m[12] = x; matrix.m[13] = y; matrix.m[14] = z;
    return matrix;
}
bool Near(float left, float right) { return std::fabs(left - right) < 0.0001F; }
bool SameMatrices(const std::vector<NeoEngine::Mat4>& left, const std::vector<NeoEngine::Mat4>& right) {
    if (left.size() != right.size()) return false;
    for (size_t matrix = 0U; matrix < left.size(); ++matrix) for (size_t index = 0U; index < 16U; ++index) if (!Near(left[matrix].m[index], right[matrix].m[index])) return false;
    return true;
}
}

int main() {
    using namespace NeoEngine;
    Skeleton skeleton;
    Bone root{"root", -1}; root.localBindPose = Translation(1.0F, 0.0F, 0.0F);
    Bone spine{"spine", 0}; spine.localBindPose = Translation(0.0F, 2.0F, 0.0F);
    Bone head{"head", 1}; head.localBindPose = Translation(0.0F, 0.0F, 3.0F);
    if (!skeleton.TryAddBone(root) || !skeleton.TryAddBone(spine) || !skeleton.TryAddBone(head) || !skeleton.IsComplete() || skeleton.RootIndex() != 0 || skeleton.GetBoneCount() != 3U || skeleton.GetBone(2U) == nullptr || skeleton.GetBone(2U)->GetParent() != 1 || skeleton.GetBone(3U) != nullptr) return 1;
    std::vector<Mat4> globalBind;
    if (!skeleton.EvaluateGlobalBindPose(globalBind) || globalBind.size() != 3U || !Near(globalBind[0].m[12], 1.0F) || !Near(globalBind[1].m[12], 1.0F) || !Near(globalBind[1].m[13], 2.0F) || !Near(globalBind[2].m[12], 1.0F) || !Near(globalBind[2].m[13], 2.0F) || !Near(globalBind[2].m[14], 3.0F)) return 1;
    if (!skeleton.DeriveInverseBindPose()) return 1;
    const Bone* derivedHead = skeleton.GetBone(2U);
    if (derivedHead == nullptr || !Near(derivedHead->inverseBindPose.m[12], -1.0F) || !Near(derivedHead->inverseBindPose.m[13], -2.0F) || !Near(derivedHead->inverseBindPose.m[14], -3.0F)) return 1;
    const Mat4 bindIdentity = MulMat4(globalBind[2U], derivedHead->inverseBindPose);
    if (!Near(bindIdentity.m[0], 1.0F) || !Near(bindIdentity.m[5], 1.0F) || !Near(bindIdentity.m[10], 1.0F) || !Near(bindIdentity.m[12], 0.0F) || !Near(bindIdentity.m[13], 0.0F) || !Near(bindIdentity.m[14], 0.0F)) return 1;
    std::vector<Mat4> localPose; for (const Bone& bone : skeleton.GetBones()) localPose.push_back(bone.localBindPose);
    std::vector<Mat4> palette;
    if (!skeleton.EvaluateSkinningPalette(localPose, palette) || palette.size() != 3U || !Near(palette[2U].m[0], 1.0F) || !Near(palette[2U].m[5], 1.0F) || !Near(palette[2U].m[10], 1.0F) || !Near(palette[2U].m[12], 0.0F) || !Near(palette[2U].m[13], 0.0F) || !Near(palette[2U].m[14], 0.0F)) return 1;
    localPose[2U].m[14] = 4.0F;
    if (!skeleton.EvaluateSkinningPalette(localPose, palette) || !Near(palette[2U].m[14], 1.0F)) return 1;
    std::vector<float> skinnedPosition{1.0F, 2.0F, 3.0F};
    VertexWeight headInfluence{}; headInfluence.boneIDs[0] = 2; headInfluence.weights[0] = 1.0F;
    SkinningError skinningError = SkinningError::None;
    if (!Skinning::ApplySkinning(skinnedPosition, std::vector<VertexWeight>{headInfluence}, palette, &skinningError) || skinningError != SkinningError::None || !Near(skinnedPosition[0], 1.0F) || !Near(skinnedPosition[1], 2.0F) || !Near(skinnedPosition[2], 4.0F)) return 1;
    const std::vector<float> stableSkinnedPosition = skinnedPosition;
    headInfluence.boneIDs[0] = 3;
    if (Skinning::ApplySkinning(skinnedPosition, std::vector<VertexWeight>{headInfluence}, palette, &skinningError) || skinningError != SkinningError::InvalidBoneIndex || skinnedPosition != stableSkinnedPosition) return 1;
    const std::vector<Mat4> stablePalette = palette;
    if (skeleton.EvaluateSkinningPalette(std::vector<Mat4>{localPose[0]}, palette) || skeleton.LastError() != SkeletonError::PoseCountMismatch || !SameMatrices(palette, stablePalette)) return 1;
    localPose[1U].m[3] = 0.25F;
    if (skeleton.EvaluateSkinningPalette(localPose, palette) || skeleton.LastError() != SkeletonError::InvalidPose || !SameMatrices(palette, stablePalette)) return 1;
    localPose[1U].m[3] = 0.0F;
    Skeleton unavailable; if (!unavailable.TryAddBone(Bone{"root", -1}) || unavailable.EvaluateSkinningPalette(std::vector<Mat4>{Translation(0.0F, 0.0F, 0.0F)}, palette) || unavailable.LastError() != SkeletonError::InverseBindUnavailable || !SameMatrices(palette, stablePalette)) return 1;
    const size_t stableCount = skeleton.GetBoneCount();
    if (skeleton.TryAddBone(Bone{"head", 1}) || skeleton.LastError() != SkeletonError::DuplicateName || skeleton.GetBoneCount() != stableCount) return 1;
    if (skeleton.TryAddBone(Bone{"second_root", -1}) || skeleton.LastError() != SkeletonError::MultipleRoots || skeleton.GetBoneCount() != stableCount) return 1;
    if (skeleton.TryAddBone(Bone{"forward", 7}) || skeleton.LastError() != SkeletonError::InvalidParent || skeleton.GetBoneCount() != stableCount) return 1;
    Skeleton invalidFirst;
    if (invalidFirst.TryAddBone(Bone{"not_root", 0}) || invalidFirst.LastError() != SkeletonError::RootRequired || invalidFirst.GetBoneCount() != 0U) return 1;
    Skeleton capacity;
    if (!capacity.TryAddBone(Bone{"root", -1})) return 1;
    for (size_t index = 1U; index < Skeleton::kMaxBones; ++index) if (!capacity.TryAddBone(Bone{"bone_" + std::to_string(index), 0})) return 1;
    if (capacity.TryAddBone(Bone{"overflow", 0}) || capacity.LastError() != SkeletonError::CapacityExceeded || capacity.GetBoneCount() != Skeleton::kMaxBones) return 1;
    Skeleton invalidPose; Bone invalidRoot{"root", -1}; invalidRoot.localBindPose = Translation(0.0F, 0.0F, 0.0F); invalidRoot.localBindPose.m[0] = std::numeric_limits<float>::quiet_NaN();
    if (!invalidPose.TryAddBone(invalidRoot)) return 1;
    const std::vector<Mat4> stableOutput{Translation(9.0F, 0.0F, 0.0F)};
    globalBind = stableOutput;
    if (invalidPose.EvaluateGlobalBindPose(globalBind) || invalidPose.LastError() != SkeletonError::InvalidBindPose || globalBind.size() != 1U || !Near(globalBind[0].m[12], 9.0F)) return 1;
    Skeleton empty;
    if (empty.EvaluateGlobalBindPose(globalBind) || empty.LastError() != SkeletonError::EmptySkeleton || globalBind.size() != 1U || !Near(globalBind[0].m[12], 9.0F)) return 1;
    Skeleton singular; Bone singularRoot{"root", -1}; singularRoot.localBindPose = Translation(0.0F, 0.0F, 0.0F); singularRoot.localBindPose.m[0] = 0.0F;
    if (!singular.TryAddBone(singularRoot) || !singular.DeriveInverseBindPose()) { if (singular.LastError() != SkeletonError::SingularBindPose) return 1; }
    if (singular.LastError() != SkeletonError::SingularBindPose || !Near(singular.GetBone(0U)->inverseBindPose.m[0], 1.0F)) return 1;
    std::printf("SKELETON_SMOKE_OK bones=3 hierarchy=1 inverseBind=1 palette=1 skinningPath=1 singleRoot=1 bounded=64 atomic=1\n");
    return 0;
}
