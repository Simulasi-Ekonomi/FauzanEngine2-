#include "HierarchicalClusterRenderer.h"
#include <iostream>

void HierarchicalClusterRenderer::AddCluster(const ClusterNode& node)
{
    clusters.push_back(node);
}

void HierarchicalClusterRenderer::RenderVisible()
{
    for(const auto& c : clusters)
    {
        std::cout<<"Rendering cluster "<<c.id<<"\n";
    }
}
