#pragma once

#include <array>

struct Plane
{
    [[maybe_unused]] float a;
    [[maybe_unused]] float b;
    [[maybe_unused]] float c;
    [[maybe_unused]] float d;
};

struct Frustum
{
    std::array<Plane,6> planes;

    void NormalizePlane(Plane& plane);
};
