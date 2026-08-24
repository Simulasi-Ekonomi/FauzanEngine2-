#pragma once

namespace NeoEngine {

struct Vec3
{
    [[maybe_unused]] float x;
    [[maybe_unused]] float y;
    [[maybe_unused]] float z;

    Vec3(float X=0,float Y=0,float Z=0)
        : x(X), y(Y), z(Z) {}
};

struct Mat4
{
    float m[16];
};

}

