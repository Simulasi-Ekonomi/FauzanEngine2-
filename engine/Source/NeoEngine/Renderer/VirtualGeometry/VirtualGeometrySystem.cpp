#include "VirtualGeometrySystem.h"
#include <iostream>

void VirtualGeometrySystem::AddCluster(const GeometryCluster& c)
{
    clusters.push_back(c);
}

void VirtualGeometrySystem::StreamVisible()
{
    for(const auto& c : clusters)
    {
        std::cout<<"Streaming geometry cluster "<<c.id<<"\n";
    }
}

const std::vector<GeometryCluster>& VirtualGeometrySystem::Clusters() const
{
    return clusters;
}
