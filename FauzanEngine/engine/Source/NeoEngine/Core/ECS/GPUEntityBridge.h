#pragma once

#include <vector>
#include <cstdint>

struct GPUEntity
{
    [[maybe_unused]] float x;
    [[maybe_unused]] float y;
    [[maybe_unused]] float vx;
    [[maybe_unused]] float vy;
};

class GPUEntityBridge
{

private:

    [[maybe_unused]] std::vector<GPUEntity> gpuEntities;

public:

    void Upload(float x,float y,float vx,float vy)
    {

        gpuEntities.push_back({x,y,vx,vy});

    }

    GPUEntity* Data()
    {
        return gpuEntities.data();
    }

    uint32_t Size() const
    {
        return gpuEntities.size();
    }

};
