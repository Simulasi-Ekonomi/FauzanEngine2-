#include "MemoryAllocator.h"
#include "EngineAllocator.h"
#include <cstdlib>

namespace NeoEngine {

void* MemoryAllocator::Allocate(size_t size) {
    return EngineAllocator::Allocate(size);
}

void MemoryAllocator::Free(void* ptr) {
    EngineAllocator::Deallocate(ptr);
}

void* MemoryAllocator::Reallocate(void* ptr, size_t newSize) {
    return EngineAllocator::Reallocate(ptr, newSize);
}

size_t MemoryAllocator::GetAllocationSize(void* ptr) {
    // Implementation bisa menggunakan malloc_usable_size atau tracking internal
    return 0;
}

} // namespace NeoEngine
