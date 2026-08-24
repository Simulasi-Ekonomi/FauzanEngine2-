#include "MeshletRenderer.h"
#include <iostream>

void MeshletRenderer::AddMeshlet(const Meshlet& meshlet)
{
    meshlets.push_back(meshlet);
}

void MeshletRenderer::Render()
{
    for(const auto& m : meshlets)
    {
        std::cout << "Rendering meshlet with "
                  << m.triangleCount
                  << " triangles\n";
    }
}
