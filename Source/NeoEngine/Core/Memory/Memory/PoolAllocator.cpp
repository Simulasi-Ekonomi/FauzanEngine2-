#include "PoolAllocator.h"
#include <cstdlib>
#include <stdexcept>

namespace NeoEngine {

PoolAllocator::PoolAllocator(size_t blockSize, size_t blockCount)
    : BlockSize(blockSize),
      BlockCount(blockCount),
      memory(nullptr),
      freeList(nullptr)
{
    Initialize();
}

void PoolAllocator::Initialize()
{
    size_t totalSize = BlockSize * BlockCount;

    memory = std::malloc(totalSize);

    if (!memory)  // FIX: cek null sebelum lanjut
        throw std::bad_alloc();

    char* ptr = static_cast<char*>(memory);

    freeList = nullptr;

    for (size_t i = 0; i < BlockCount; i++)
    {
        FreeBlock* block = reinterpret_cast<FreeBlock*>(ptr + i * BlockSize);
        block->next = freeList;
        freeList = block;
    }
}

void* PoolAllocator::Allocate()
{
    if (!freeList)
        return nullptr;

    FreeBlock* block = freeList;
    freeList = block->next;
    return block;
}

void PoolAllocator::Free(void* ptr)
{
    if (!ptr)
        return;

    FreeBlock* block = static_cast<FreeBlock*>(ptr);
    block->next = freeList;
    freeList = block;
}

} // namespace NeoEngine
