#pragma once

#include <vector>
#include <cstdint>

namespace NeoEngine
{

struct GPUBuffer
{
    uint32_t id;
    size_t size;
};

class GPUBufferPool
{
public:

    GPUBuffer Allocate(size_t bytes);

    void Free(const GPUBuffer& buffer);

private:

    [[maybe_unused]] std::vector<GPUBuffer> freeBuffers;

};

}
