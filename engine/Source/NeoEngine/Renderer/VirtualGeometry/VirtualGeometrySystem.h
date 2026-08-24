#pragma once
#include <vector>

struct GeometryCluster
{
    int id;
    int triangleCount;
};

class VirtualGeometrySystem
{
public:

    void AddCluster(const GeometryCluster& c);

    void StreamVisible();

    const std::vector<GeometryCluster>& Clusters() const;

private:

    std::vector<GeometryCluster> clusters;
};
