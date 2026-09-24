#include "Core/Debug/MemoryTracker.h"
#include <cassert>
#include <cstring>
int main() {
    using NeoEngine::MemoryTracker;
    const auto before = MemoryTracker::GetLiveAllocationCount();
    void* p = MemoryTracker::Allocate(64, __FILE__, __LINE__);
    assert(p && MemoryTracker::GetLiveAllocationCount() == before + 1);
    std::memset(p, 0x5A, 64);
    p = MemoryTracker::Reallocate(p, 128, __FILE__, __LINE__);
    assert(p && MemoryTracker::GetTotalAllocated() >= 128);
    MemoryTracker::Deallocate(p);
    assert(MemoryTracker::GetLiveAllocationCount() == before);
    const std::string report = MemoryTracker::ReportLeaks();
    assert(report.find("live=") != std::string::npos);
    assert(MemoryTracker::GetPeakAllocated() >= 128);
    return 0;
}
