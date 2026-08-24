#pragma once
#include <vector>

namespace NeoEngine {

struct Vertex {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
    Vertex() = default;
};

struct VertexWeight {
    int boneID;
    float weight;
};

} // namespace NeoEngine
