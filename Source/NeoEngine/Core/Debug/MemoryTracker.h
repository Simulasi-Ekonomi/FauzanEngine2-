#pragma once
#include <cstddef>

namespace NeoEngine {

class MemoryTracker {
public:
    static void* Allocate(size_t size, const char* file, int line);
    static void Deallocate(void* ptr);
    static void* Reallocate(void* ptr, size_t newSize, const char* file, int line);

    static void ReportLeaks();

    static size_t GetTotalAllocated();
    static size_t GetPeakAllocated();
    static size_t GetAllocationCount();

private:
    static size_t totalAllocated;
    static size_t peakAllocated;
    static size_t allocationCount;
};

}
