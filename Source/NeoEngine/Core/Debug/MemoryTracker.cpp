#include "MemoryTracker.h"
#include <cstdlib>

#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace NeoEngine {

size_t MemoryTracker::totalAllocated = 0;
size_t MemoryTracker::peakAllocated = 0;
size_t MemoryTracker::allocationCount = 0;

void* MemoryTracker::Allocate(size_t size, const char*, int) {
    totalAllocated += size;
    allocationCount++;
    if (totalAllocated > peakAllocated) peakAllocated = totalAllocated;
    return malloc(size);
}

void MemoryTracker::Deallocate(void* ptr) {
    free(ptr);
}

void* MemoryTracker::Reallocate(void* ptr, size_t newSize, const char*, int) {
    return realloc(ptr, newSize);
}

void MemoryTracker::ReportLeaks() {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "MemoryTracker", "Report not fully implemented");
#endif
}

size_t MemoryTracker::GetTotalAllocated() { return totalAllocated; }
size_t MemoryTracker::GetPeakAllocated() { return peakAllocated; }
size_t MemoryTracker::GetAllocationCount() { return allocationCount; }

}
