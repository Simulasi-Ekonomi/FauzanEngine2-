#pragma once
#include <vector>

struct GPUResourceHandle
{
    uint32_t id;
};

class BindlessResourceManager
{
public:

    GPUResourceHandle RegisterResource();

    void RemoveResource(uint32_t id);

    size_t ResourceCount() const;

private:

    std::vector<GPUResourceHandle> resources;
};
