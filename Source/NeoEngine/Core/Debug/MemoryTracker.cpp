#include "MemoryTracker.h"
#include <cstdlib>
#include <cstdio>
#include <mutex>
#include <sstream>
#include <unordered_map>

#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace NeoEngine {
namespace {
std::mutex g_Mutex;
std::unordered_map<void*, MemoryTracker::Allocation> g_Allocations;
}

size_t MemoryTracker::totalAllocated = 0;
size_t MemoryTracker::peakAllocated = 0;
size_t MemoryTracker::allocationCount = 0;

void* MemoryTracker::Allocate(size_t size, const char* file, int line) {
    if (size == 0) size = 1;
    void* ptr = std::malloc(size);
    if (!ptr) return nullptr;
    std::lock_guard<std::mutex> lock(g_Mutex);
    g_Allocations.emplace(ptr, Allocation{size, file, line});
    totalAllocated += size;
    ++allocationCount;
    if (totalAllocated > peakAllocated) peakAllocated = totalAllocated;
    return ptr;
}

void MemoryTracker::Deallocate(void* ptr) {
    if (!ptr) return;
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        auto it = g_Allocations.find(ptr);
        if (it != g_Allocations.end()) {
            totalAllocated -= it->second.size;
            g_Allocations.erase(it);
        }
    }
    std::free(ptr);
}

void* MemoryTracker::Reallocate(void* ptr, size_t newSize, const char* file, int line) {
    if (!ptr) return Allocate(newSize, file, line);
    if (newSize == 0) {
        Deallocate(ptr);
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(g_Mutex);
    auto it = g_Allocations.find(ptr);
    if (it == g_Allocations.end()) return nullptr;

    void* resized = std::realloc(ptr, newSize);
    if (!resized) return nullptr;

    totalAllocated -= it->second.size;
    if (resized != ptr) g_Allocations.erase(it);
    g_Allocations[resized] = Allocation{newSize, file, line};
    totalAllocated += newSize;
    if (totalAllocated > peakAllocated) peakAllocated = totalAllocated;
    return resized;
}

std::string MemoryTracker::ReportLeaks() {
    std::lock_guard<std::mutex> lock(g_Mutex);
    std::ostringstream out;
    out << "MemoryTracker: live=" << g_Allocations.size()
        << " bytes=" << totalAllocated << '\n';
    for (const auto& [ptr, allocation] : g_Allocations) {
        out << "  ptr=" << ptr << " size=" << allocation.size
            << " at=" << (allocation.file ? allocation.file : "<unknown>")
            << ':' << allocation.line << '\n';
    }
    return out.str();
}

size_t MemoryTracker::GetTotalAllocated() {
    std::lock_guard<std::mutex> lock(g_Mutex);
    return totalAllocated;
}
size_t MemoryTracker::GetPeakAllocated() {
    std::lock_guard<std::mutex> lock(g_Mutex);
    return peakAllocated;
}
size_t MemoryTracker::GetAllocationCount() {
    std::lock_guard<std::mutex> lock(g_Mutex);
    return allocationCount;
}
size_t MemoryTracker::GetLiveAllocationCount() {
    std::lock_guard<std::mutex> lock(g_Mutex);
    return g_Allocations.size();
}

}
