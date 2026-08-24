#pragma once
#include <vector>

struct ClusterNode
{
    int id;
    int parent;
    int triangleCount;
};

class HierarchicalClusterRenderer
{
public:

    void AddCluster(const ClusterNode& node);

    void RenderVisible();

private:

    std::vector<ClusterNode> clusters;
};
