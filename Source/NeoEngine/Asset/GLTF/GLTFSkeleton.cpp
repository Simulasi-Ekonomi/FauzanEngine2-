#include "GLTFSkeleton.h"

void Skeleton::AddBone(const Bone& bone) {
    bones.push_back(bone);
}

bool Skeleton::AddBoneChecked(const Bone& bone) {
    if (bone.parentIndex < -1 || bone.parentIndex >= static_cast<int>(bones.size())) return false;
    for (float value : bone.inverseBindMatrix) if (!std::isfinite(value)) return false;
    bones.push_back(bone);
    return true;
}

bool Skeleton::Validate() const noexcept {
    if (bones.empty()) return false;
    for (std::size_t i = 0; i < bones.size(); ++i) {
        const Bone& bone = bones[i];
        if (bone.parentIndex < -1 || bone.parentIndex >= static_cast<int>(i)) return false;
        for (float value : bone.inverseBindMatrix) if (!std::isfinite(value)) return false;
    }
    return true;
}