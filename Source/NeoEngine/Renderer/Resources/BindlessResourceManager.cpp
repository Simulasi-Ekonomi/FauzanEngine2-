#include "BindlessResourceManager.h"

GPUResourceHandle BindlessResourceManager::RegisterResource()
{
    GPUResourceHandle handle;
    handle.id = resources.size();

    resources.push_back(handle);

    return handle;
}

void BindlessResourceManager::RemoveResource(uint32_t id)
{
    if(id < resources.size())
    {
        resources[id].id = 0;
    }
}

size_t BindlessResourceManager::ResourceCount() const
{
    return resources.size();
}
