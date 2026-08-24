#include "ClusterLighting.h"

void ClusterLighting::AddLight(const Light& light)
{
    lights.push_back(light);
}

void ClusterLighting::BuildClusters(int clusterCount)
{
    clusters.clear();
    clusters.resize(clusterCount);

    for(size_t i=0;i<lights.size();i++)
    {
        int clusterIndex = i % clusterCount;
        clusters[clusterIndex].lights.push_back(i);
    }
}

const std::vector<Cluster>& ClusterLighting::GetClusters() const
{
    return clusters;
}
