#pragma once
#include <cstddef>

namespace NeoEngine {

class MemoryManager {
public:
    static void Init();
    static void* Allocate(size_t size);
    static void Free(void* ptr);

    static size_t GetTotalAllocated();
    static size_t GetPeakAllocated();

private:
    static size_t s_TotalAllocated;
    static size_t s_PeakAllocated;
};

}
