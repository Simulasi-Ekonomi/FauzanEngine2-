#include "MemoryTracker.h"
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <unordered_map>

namespace NeoEngine {
namespace {
struct Allocation { std::size_t size = 0; const char* file = nullptr; int line = 0; };
std::unordered_map<void*, Allocation> allocations;
std::mutex allocationsMutex;
}
std::size_t MemoryTracker::totalAllocated = 0;
std::size_t MemoryTracker::peakAllocated = 0;
std::size_t MemoryTracker::allocationCount = 0;
std::size_t MemoryTracker::liveAllocated = 0;

void* MemoryTracker::Allocate(std::size_t size, const char* file, int line) {
    void* ptr = std::malloc(size);
    if (!ptr) return nullptr;
    std::lock_guard lock(allocationsMutex);
    allocations.emplace(ptr, Allocation{size, file, line});
    liveAllocated += size;
    totalAllocated += size;
    if (liveAllocated > peakAllocated) peakAllocated = liveAllocated;
    ++allocationCount;
    return ptr;
}
void MemoryTracker::Deallocate(void* ptr) {
    if (!ptr) return;
    { std::lock_guard lock(allocationsMutex);
      auto it = allocations.find(ptr);
      if (it != allocations.end()) { liveAllocated -= it->second.size; allocations.erase(it); }
    }
    std::free(ptr);
}
void* MemoryTracker::Reallocate(void* ptr, std::size_t newSize, const char* file, int line) {
    if (!ptr) return Allocate(newSize, file, line);
    if (newSize == 0) { Deallocate(ptr); return nullptr; }
    std::lock_guard lock(allocationsMutex);
    auto it = allocations.find(ptr);
    if (it == allocations.end()) {
        void* replacement = std::realloc(ptr, newSize);
        if (!replacement) return nullptr;
        allocations.emplace(replacement, Allocation{newSize, file, line});
        liveAllocated += newSize; totalAllocated += newSize;
        if (liveAllocated > peakAllocated) peakAllocated = liveAllocated;
        ++allocationCount;
        return replacement;
    }
    const auto oldSize = it->second.size;
    void* replacement = std::realloc(ptr, newSize);
    if (!replacement) return nullptr;
    liveAllocated = liveAllocated - oldSize + newSize;
    totalAllocated += newSize;
    if (liveAllocated > peakAllocated) peakAllocated = liveAllocated;
    allocations.erase(it);
    allocations.emplace(replacement, Allocation{newSize, file, line});
    return replacement;
}
std::string MemoryTracker::ReportLeaks() {
    std::lock_guard lock(allocationsMutex);
    std::ostringstream report;
    report << "live=" << allocations.size() << " bytes=" << liveAllocated
           << " total=" << totalAllocated << " peak=" << peakAllocated << '\n';
    for (const auto& [ptr, allocation] : allocations)
        report << "ptr=" << ptr << " size=" << allocation.size
               << " file=" << (allocation.file ? allocation.file : "<unknown>")
               << " line=" << allocation.line << '\n';
    return report.str();
}
std::size_t MemoryTracker::GetTotalAllocated() { std::lock_guard lock(allocationsMutex); return totalAllocated; }
std::size_t MemoryTracker::GetPeakAllocated() { std::lock_guard lock(allocationsMutex); return peakAllocated; }
std::size_t MemoryTracker::GetAllocationCount() { std::lock_guard lock(allocationsMutex); return allocationCount; }
std::size_t MemoryTracker::GetLiveAllocationCount() { std::lock_guard lock(allocationsMutex); return allocations.size(); }
}
