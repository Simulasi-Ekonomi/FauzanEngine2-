#pragma once

#include "Vec4.h"
#include "Mat4.h"

namespace Neo
{

struct Transform
{

    Vec4 position;
    Vec4 rotation;
    Vec4 scale;

    Transform()
        : position(0,0,0),
          rotation(0,0,0),
          scale(1,1,1)
    {
    }

    Mat4 ToMatrix() const
    {

        Mat4 m;

        m.rows[3] = Vec4(position.x, position.y, position.z, 1.0f);

        return m;

    }

};

}
