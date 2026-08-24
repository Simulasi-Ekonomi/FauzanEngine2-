#include "GPUOcclusionCulling.h"

void GPUOcclusionCulling::AddObject(const OcclusionObject& obj)
{
    objects.push_back(obj);
}

void GPUOcclusionCulling::PerformCulling()
{
    visible.clear();

    for(size_t i=0;i<objects.size();i++)
    {
        if(objects[i].size > 0.1f)
        {
            visible.push_back(i);
        }
    }
}

const std::vector<int>& GPUOcclusionCulling::VisibleObjects() const
{
    return visible;
}
