#pragma once
#include <cstddef>
#include <string>

namespace NeoEngine {

class MemoryTracker {
public:
    struct Allocation { size_t size; const char* file; int line; };

    static void* Allocate(size_t size, const char* file, int line);
    static void Deallocate(void* ptr);
    static void* Reallocate(void* ptr, size_t newSize, const char* file, int line);

    static std::string ReportLeaks();
    static size_t GetTotalAllocated();
    static size_t GetPeakAllocated();
    static size_t GetAllocationCount();
    static size_t GetLiveAllocationCount();

private:
    static size_t totalAllocated;
    static size_t peakAllocated;
    static size_t allocationCount;
};

}
