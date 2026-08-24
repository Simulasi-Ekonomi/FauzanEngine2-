#pragma once
#include <vector>

struct Light
{
    float x;
    float y;
    float z;
    float radius;
};

struct Cluster
{
    std::vector<int> lights;
};

class ClusterLighting
{
public:

    void AddLight(const Light& light);

    void BuildClusters(int clusterCount);

    const std::vector<Cluster>& GetClusters() const;

private:

    std::vector<Light> lights;
    std::vector<Cluster> clusters;
};
