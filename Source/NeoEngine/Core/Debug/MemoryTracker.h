#pragma once
#include <cstddef>
#include <string>

namespace NeoEngine {
class MemoryTracker {
public:
    static void* Allocate(std::size_t size, const char* file, int line);
    static void Deallocate(void* ptr);
    static void* Reallocate(void* ptr, std::size_t newSize, const char* file, int line);
    [[nodiscard]] static std::string ReportLeaks();
    [[nodiscard]] static std::size_t GetTotalAllocated();
    [[nodiscard]] static std::size_t GetPeakAllocated();
    [[nodiscard]] static std::size_t GetAllocationCount();
    [[nodiscard]] static std::size_t GetLiveAllocationCount();
private:
    MemoryTracker() = delete;
    static std::size_t totalAllocated;
    static std::size_t peakAllocated;
    static std::size_t allocationCount;
    static std::size_t liveAllocated;
};
}
