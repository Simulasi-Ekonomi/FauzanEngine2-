#include "MemoryManager.h"
#include <cstdlib>
#include <iostream>

std::atomic<size_t> MemoryManager::totalAllocatedBytes{0};
std::atomic<size_t> MemoryManager::activeAllocationCount{0};
std::unordered_map<void*, size_t> MemoryManager::allocationMap;
std::mutex MemoryManager::allocationMutex;

void MemoryManager::Init() {
    std::lock_guard<std::mutex> lock(allocationMutex);
    totalAllocatedBytes = 0;
    activeAllocationCount = 0;
    allocationMap.clear();
}

void MemoryManager::Shutdown() {
    std::lock_guard<std::mutex> lock(allocationMutex);
    if (!allocationMap.empty()) {
        std::cerr << "[MemoryManager] Memory leak detected! Active allocations: "
                  << allocationMap.size() << " (" << totalAllocatedBytes << " bytes)\n";
    }
}

void* MemoryManager::Allocate(size_t size) {
    if (size == 0) return nullptr;
    void* ptr = std::malloc(size);
    if (ptr) {
        std::lock_guard<std::mutex> lock(allocationMutex);
        allocationMap[ptr] = size;
        totalAllocatedBytes += size;
        activeAllocationCount++;
    }
    return ptr;
}

void MemoryManager::Free(void* ptr) {
    if (!ptr) return;
    std::lock_guard<std::mutex> lock(allocationMutex);
    auto it = allocationMap.find(ptr);
    if (it != allocationMap.end()) {
        totalAllocatedBytes -= it->second;
        activeAllocationCount--;
        allocationMap.erase(it);
    }
    std::free(ptr);
}

size_t MemoryManager::GetTotalAllocatedBytes() {
    return totalAllocatedBytes.load();
}

size_t MemoryManager::GetActiveAllocationCount() {
    return activeAllocationCount.load();
}

bool MemoryManager::HasLeaks() {
    std::lock_guard<std::mutex> lock(allocationMutex);
    return !allocationMap.empty();
}
