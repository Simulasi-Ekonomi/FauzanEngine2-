#pragma once
#include <vector>

struct Meshlet
{
    uint32_t vertexOffset;
    uint32_t vertexCount;
    uint32_t triangleOffset;
    uint32_t triangleCount;
};

class MeshletRenderer
{
public:

    void AddMeshlet(const Meshlet& meshlet);

    void Render();

private:

    std::vector<Meshlet> meshlets;
};
