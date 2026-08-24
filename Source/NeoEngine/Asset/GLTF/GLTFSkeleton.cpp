#include <cassert>
#include "GLTFSkeleton.h"

void Skeleton::AddBone(const Bone& bone)
{
    bones.push_back(bone);
}

const std::vector<Bone>& Skeleton::GetBones() const
{
    return bones;
}
