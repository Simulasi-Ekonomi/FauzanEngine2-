#pragma once
#include <cstddef>
#include <cstdint>

namespace NeoEngine {

class PoolAllocator {
public:
    struct FreeNode { FreeNode* next; };

    PoolAllocator(size_t blockSize, size_t blockCount);
    ~PoolAllocator();

    void* Allocate();
    void Free(void* ptr);
    size_t GetBlockSize() const { return blockSize_; }

private:
    void* memory_ = nullptr;
    FreeNode* freeList_ = nullptr;
    size_t blockSize_;
    size_t blockCount_;
};

} // namespace
