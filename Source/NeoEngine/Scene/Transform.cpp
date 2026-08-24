#include <cassert>
#include "Transform.h"

std::array<float,16> Transform::LocalMatrix() const
{
    std::array<float,16> m{};

    m[0]=scale[0];
    m[5]=scale[1];
    m[10]=scale[2];
    m[15]=1.0f;

    m[12]=position[0];
    m[13]=position[1];
    m[14]=position[2];

    return m;
}
