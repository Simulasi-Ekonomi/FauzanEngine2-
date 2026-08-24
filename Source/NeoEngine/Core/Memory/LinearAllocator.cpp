#include "LinearAllocator.h"
#include <cstdlib>

namespace NeoEngine {

LinearAllocator::LinearAllocator(size_t size) {
    memory_ = static_cast<uint8_t*>(malloc(size));
    totalSize_ = size;
    offset_ = 0;
}

LinearAllocator::~LinearAllocator() {
    free(memory_);
}

void* LinearAllocator::Allocate(size_t size, size_t alignment) {
    size_t current = reinterpret_cast<size_t>(memory_ + offset_);
    size_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t newOffset = aligned - reinterpret_cast<size_t>(memory_) + size;
    if (newOffset > totalSize_) return nullptr;
    offset_ = newOffset;
    return reinterpret_cast<void*>(aligned);
}

void LinearAllocator::Reset() {
    offset_ = 0;
}

} // namespace
