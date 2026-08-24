#include "MemoryManager.h"
#include <cstdlib>

namespace NeoEngine {

size_t MemoryManager::s_TotalAllocated = 0;
size_t MemoryManager::s_PeakAllocated = 0;

void MemoryManager::Init() {
    s_TotalAllocated = 0;
    s_PeakAllocated = 0;
}

void* MemoryManager::Allocate(size_t size) {
    void* ptr = std::malloc(size);
    if (ptr) {
        s_TotalAllocated += size;
        if (s_TotalAllocated > s_PeakAllocated)
            s_PeakAllocated = s_TotalAllocated;
    }
    return ptr;
}

void MemoryManager::Free(void* ptr) {
    std::free(ptr);
}

size_t MemoryManager::GetTotalAllocated() {
    return s_TotalAllocated;
}

size_t MemoryManager::GetPeakAllocated() {
    return s_PeakAllocated;
}

}
