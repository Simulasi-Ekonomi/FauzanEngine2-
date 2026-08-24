#include "PoolAllocator.h"
#include <cstdlib>
#include <cstddef>

namespace NeoEngine {

PoolAllocator::PoolAllocator(size_t blockSize, size_t blockCount)
    : blockSize_(blockSize), blockCount_(blockCount) {
    size_t total = blockSize * blockCount;
    memory_ = malloc(total);
    // Build free list
    for (size_t i = 0; i < blockCount; ++i) {
        FreeNode* node = reinterpret_cast<FreeNode*>(
            static_cast<uint8_t*>(memory_) + i * blockSize);
        node->next = freeList_;
        freeList_ = node;
    }
}

PoolAllocator::~PoolAllocator() {
    free(memory_);
}

void* PoolAllocator::Allocate() {
    if (!freeList_) return nullptr;
    FreeNode* node = freeList_;
    freeList_ = node->next;
    return node;
}

void PoolAllocator::Free(void* ptr) {
    if (!ptr) return;
    FreeNode* node = static_cast<FreeNode*>(ptr);
    node->next = freeList_;
    freeList_ = node;
}

} // namespace
