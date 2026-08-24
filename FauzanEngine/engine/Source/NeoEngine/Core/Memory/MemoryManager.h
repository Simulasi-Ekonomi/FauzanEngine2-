#pragma once
#include <cstddef>

class MemoryManager {
public:
    static void Init();
    static void Shutdown();
    static void* Allocate(size_t size);
    static void Free(void* ptr);
};
