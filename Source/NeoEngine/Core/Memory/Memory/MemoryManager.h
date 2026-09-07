#pragma once
#include <cstddef>
#include <atomic>
#include <unordered_map>
#include <mutex>

class MemoryManager {
private:
    static std::atomic<size_t> totalAllocatedBytes;
    static std::atomic<size_t> activeAllocationCount;
    static std::unordered_map<void*, size_t> allocationMap;
    static std::mutex allocationMutex;

public:
    static void Init();
    static void Shutdown();
    static void* Allocate(size_t size);
    static void Free(void* ptr);
    static size_t GetTotalAllocatedBytes();
    static size_t GetActiveAllocationCount();
    static bool HasLeaks();
};
