#pragma once
#include <vector>
#include <cstdint>

namespace NeoEngine {

using EntityID = uint32_t;

struct PositionComponent {
    std::vector<float> x;
    std::vector<float> y;
    std::vector<float> z;
};

struct VelocityComponent {
    std::vector<float> vx;
    std::vector<float> vy;
    std::vector<float> vz;
};

struct ColliderComponent {
    std::vector<float> radius;
    std::vector<float> invMass;
};

struct MeshComponent {
    std::vector<uint32_t> meshID;
};

struct RotationComponent {
    std::vector<float> x;
    std::vector<float> y;
    std::vector<float> z;
};

} // namespace NeoEngine
