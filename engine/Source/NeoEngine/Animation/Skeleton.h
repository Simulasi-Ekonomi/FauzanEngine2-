#pragma once

#include <vector>
#include "Bone.h"

namespace NeoEngine
{

class Skeleton
{
public:

    void AddBone(const Bone& bone);

    const std::vector<Bone>& GetBones() const;

    size_t GetBoneCount() const;

private:

    [[maybe_unused]] std::vector<Bone> bones;

};

}
