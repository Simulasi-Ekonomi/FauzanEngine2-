#include "GPUFrustumCulling.h"

void GPUFrustumCulling::AddObject(const BoundingSphere& sphere)
{
    objects.push_back(sphere);
}

void GPUFrustumCulling::PerformCulling()
{
    visible.clear();

    for(size_t i=0;i<objects.size();i++)
    {
        if(objects[i].radius > 0.0f)
        {
            visible.push_back(i);
        }
    }
}

const std::vector<int>& GPUFrustumCulling::VisibleObjects() const
{
    return visible;
}
