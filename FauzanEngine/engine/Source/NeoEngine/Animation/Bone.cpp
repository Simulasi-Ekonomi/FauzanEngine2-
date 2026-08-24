#include <cassert>
#include "Bone.h"

namespace NeoEngine
{

Bone::Bone(const std::string& name, int parent)
    : name(name)
    , parentIndex(parent)
    , localBindPose{}
    , inverseBindPose{}
{
    // Zero-init dulu, set identity manual
    // Mat4 pakai float[16] row-major
    auto makeIdentity = [](float m[16]) {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    };
    makeIdentity(localBindPose.m);
    makeIdentity(inverseBindPose.m);
}

const std::string& Bone::GetName() const
{
    return name;
}

int Bone::GetParent() const
{
    return parentIndex;
}

} // namespace NeoEngine
