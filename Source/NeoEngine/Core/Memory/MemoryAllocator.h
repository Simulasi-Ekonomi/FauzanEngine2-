#pragma once
#include <cstddef>

namespace NeoEngine {

class MemoryAllocator {
public:
    static void* Allocate(size_t size);
    static void  Free(void* ptr);
    static void* Reallocate(void* ptr, size_t newSize);
    static size_t GetAllocationSize(void* ptr);
};

} // namespace NeoEngine
