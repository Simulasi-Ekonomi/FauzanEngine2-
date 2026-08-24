#pragma once
#include <vector>

struct BoundingSphere
{
    float x;
    float y;
    float z;
    float radius;
};

class GPUFrustumCulling
{
public:

    void AddObject(const BoundingSphere& sphere);

    void PerformCulling();

    const std::vector<int>& VisibleObjects() const;

private:

    std::vector<BoundingSphere> objects;
    std::vector<int> visible;
};
