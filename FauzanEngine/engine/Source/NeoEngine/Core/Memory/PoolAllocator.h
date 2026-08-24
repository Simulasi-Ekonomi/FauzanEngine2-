#pragma once

#include <cstddef>
#include <vector>

namespace NeoEngine {

class PoolAllocator
{
public:

    PoolAllocator(size_t blockSize, size_t blockCount);

    void* Allocate();

    void Free(void* ptr);

private:

    struct FreeBlock
    {
        FreeBlock* next;
    };

    size_t BlockSize;
    size_t BlockCount;

    void* memory = nullptr;

    FreeBlock* freeList = nullptr;

    void Initialize();
};

}

