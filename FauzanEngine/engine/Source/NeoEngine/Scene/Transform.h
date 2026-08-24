#pragma once

#include <array>

struct Transform
{
    std::array<float,3> position;
    std::array<float,4> rotation;
    std::array<float,3> scale;

    std::array<float,16> LocalMatrix() const;
};
