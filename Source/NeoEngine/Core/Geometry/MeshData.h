#pragma once

#include <cstdint>
#include <vector>

namespace NeoEngine {

struct Vertex
{
    float position[3];
    float normal[3];
    float uv[2];
    // Canonical glTF skinning influences. Unskinned vertices use bone 0 with weight 1.
    std::uint32_t boneIndices[4]{};
    float boneWeights[4]{};
};

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

}
