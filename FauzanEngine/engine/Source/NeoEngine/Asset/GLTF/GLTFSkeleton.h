#include <cassert>
#pragma once
#include <vector>

struct Bone
{
    [[maybe_unused]] int parentIndex;
    float inverseBindMatrix[16];
};

class Skeleton
{
public:

    void AddBone(const Bone& bone);

    const std::vector<Bone>& GetBones() const;

private:

    [[maybe_unused]] std::vector<Bone> bones;
};
