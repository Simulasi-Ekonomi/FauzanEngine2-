#pragma once
#include <cstddef>

class MemoryAllocator {
public:
    static void* Allocate(size_t size);
    static void Free(void* ptr);
};
