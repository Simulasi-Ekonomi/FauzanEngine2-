#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

struct Bone {
    int parentIndex = -1;
    float inverseBindMatrix[16] = {
        1.0F,0.0F,0.0F,0.0F,
        0.0F,1.0F,0.0F,0.0F,
        0.0F,0.0F,1.0F,0.0F,
        0.0F,0.0F,0.0F,1.0F
    };
};

class Skeleton {
public:
    void AddBone(const Bone& bone);
    [[nodiscard]] bool AddBoneChecked(const Bone& bone);
    [[nodiscard]] const std::vector<Bone>& GetBones() const noexcept { return bones; }
    [[nodiscard]] bool Validate() const noexcept;

private:
    std::vector<Bone> bones;
};