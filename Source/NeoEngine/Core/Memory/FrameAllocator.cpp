#include "FrameAllocator.h"
#include <cstdlib>

namespace NeoEngine {

FrameAllocator::FrameAllocator(size_t capacity) : capacity_(capacity) {
    memory_ = static_cast<uint8_t*>(malloc(capacity));
}

FrameAllocator::~FrameAllocator() {
    free(memory_);
}

void* FrameAllocator::Allocate(size_t size, size_t alignment) {
    size_t current = reinterpret_cast<size_t>(memory_ + offset_);
    size_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t newOffset = aligned - reinterpret_cast<size_t>(memory_) + size;
    if (newOffset > capacity_) return nullptr;
    offset_ = newOffset;
    return reinterpret_cast<void*>(aligned);
}

void FrameAllocator::Reset() {
    offset_ = 0;
}

} // namespace
