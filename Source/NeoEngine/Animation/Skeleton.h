#pragma once

#include <cstdint>
#include <vector>
#include "Bone.h"

namespace NeoEngine
{

enum class SkeletonError : uint8_t { None, CapacityExceeded, EmptyName, DuplicateName, RootRequired, MultipleRoots, InvalidParent, EmptySkeleton, InvalidBindPose, SingularBindPose, InverseBindUnavailable, PoseCountMismatch, InvalidPose };

class Skeleton
{
public:
    static constexpr size_t kMaxBones = 64U;

    // Appends a bone only when the hierarchy remains single-rooted and topologically ordered.
    [[nodiscard]] bool TryAddBone(const Bone& bone);
    // Legacy compatibility entry point; inspect LastError after calling.
    void AddBone(const Bone& bone) { (void)TryAddBone(bone); }

    const std::vector<Bone>& GetBones() const;
    [[nodiscard]] const Bone* GetBone(size_t index) const;
    size_t GetBoneCount() const;
    // Evaluates local bind transforms in topological hierarchy order without changing output on failure.
    [[nodiscard]] bool EvaluateGlobalBindPose(std::vector<Mat4>& output);
    // Atomically derives affine inverse bind matrices for all bones from the global bind hierarchy.
    [[nodiscard]] bool DeriveInverseBindPose();
    // Evaluates caller-supplied local pose matrices into skinning palette = globalPose * inverseBind.
    [[nodiscard]] bool EvaluateSkinningPalette(const std::vector<Mat4>& localPose, std::vector<Mat4>& output);
    [[nodiscard]] int RootIndex() const { return rootIndex_; }
    [[nodiscard]] bool IsComplete() const { return !bones_.empty() && rootIndex_ == 0; }
    [[nodiscard]] SkeletonError LastError() const { return lastError_; }

private:
    std::vector<Bone> bones_;
    int rootIndex_ = -1;
    bool inverseBindReady_ = false;
    SkeletonError lastError_ = SkeletonError::None;

};

}
