#pragma once

#include <string>
#include <vector>
#include "Core/Math/Mat4.h"

namespace NeoEngine
{

class Bone
{
public:
    Bone(const std::string& name, int parent);

    const std::string& GetName()   const;
    int                GetParent() const;

    Mat4 localBindPose;
    Mat4 inverseBindPose;

private:
    std::string name;
    int         parentIndex;
};

} // namespace NeoEngine
