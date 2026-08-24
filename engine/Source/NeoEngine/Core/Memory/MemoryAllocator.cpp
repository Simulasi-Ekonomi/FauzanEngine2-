#include <cassert>
#include "MemoryAllocator.h"
#include <cstdlib>

void* MemoryAllocator::Allocate(size_t size) {
    return std::malloc(size);
}

void MemoryAllocator::Free(void* ptr) {
    std::free(ptr);
}
