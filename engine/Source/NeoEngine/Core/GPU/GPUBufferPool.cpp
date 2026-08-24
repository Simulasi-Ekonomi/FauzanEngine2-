#include <cassert>
#include "GPUBufferPool.h"
#include "GPUMemoryBudget.h"

namespace NeoEngine
{

GPUBuffer GPUBufferPool::Allocate(size_t bytes)
{
    if(!GPUMemoryBudget::Request(bytes))
        return {0,0};

    GPUBuffer buf;

    buf.id = freeBuffers.size() + 1;
    buf.size = bytes;

    return buf;
}

void GPUBufferPool::Free(const GPUBuffer& buffer)
{
    GPUMemoryBudget::Release(buffer.size);

    freeBuffers.push_back(buffer);
}

}
