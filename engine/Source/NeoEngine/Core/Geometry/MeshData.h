#pragma once

#include <vector>

namespace NeoEngine {

struct Vertex
{
    float position[3];
    float normal[3];
    float uv[2];
};

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

}
