#include <cassert>
#include "Skeleton.h"

namespace NeoEngine
{

void Skeleton::AddBone(const Bone& bone)
{
    bones.push_back(bone);
}

const std::vector<Bone>& Skeleton::GetBones() const
{
    return bones;
}

size_t Skeleton::GetBoneCount() const
{
    return bones.size();
}

}
