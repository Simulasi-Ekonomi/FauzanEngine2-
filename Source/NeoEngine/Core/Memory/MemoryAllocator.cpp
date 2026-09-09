#include "MemoryAllocator.h"
#include "EngineAllocator.h"

#include <cstddef>
#include <mutex>
#include <unordered_map>

namespace NeoEngine {
namespace {

std::mutex& AllocationMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_map<void*, std::size_t>& AllocationSizes() {
    static std::unordered_map<void*, std::size_t> sizes;
    return sizes;
}

void RecordAllocation(void* ptr, std::size_t size) {
    if (ptr == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(AllocationMutex());
    try {
        AllocationSizes()[ptr] = size;
    } catch (...) {
        // The underlying allocation succeeded, but the accounting metadata
        // could not be recorded. Release the allocation so the wrapper never
        // returns a pointer whose size cannot be tracked reliably.
        EngineAllocator::Deallocate(ptr);
        throw;
    }
}

} // namespace

void* MemoryAllocator::Allocate(size_t size) {
    void* ptr = EngineAllocator::Allocate(size);
    try {
        RecordAllocation(ptr, size);
    } catch (...) {
        throw;
    }
    return ptr;
}

void MemoryAllocator::Free(void* ptr) {
    if (ptr == nullptr) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(AllocationMutex());
        AllocationSizes().erase(ptr);
    }

    EngineAllocator::Deallocate(ptr);
}

void* MemoryAllocator::Reallocate(void* ptr, size_t newSize) {
    void* newPtr = EngineAllocator::Reallocate(ptr, newSize);

    {
        std::lock_guard<std::mutex> lock(AllocationMutex());
        try {
            auto& sizes = AllocationSizes();
            if (ptr != nullptr) {
                sizes.erase(ptr);
            }
            if (newPtr != nullptr) {
                sizes[newPtr] = newSize;
            }
        } catch (...) {
            // realloc has already transferred ownership to newPtr. Do not
            // leave an untracked live allocation if metadata insertion fails.
            if (newPtr != nullptr) {
                EngineAllocator::Deallocate(newPtr);
            }
            throw;
        }
    }

    return newPtr;
}

size_t MemoryAllocator::GetAllocationSize(void* ptr) {
    if (ptr == nullptr) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(AllocationMutex());
    const auto& sizes = AllocationSizes();
    const auto it = sizes.find(ptr);
    return it == sizes.end() ? 0 : it->second;
}

} // namespace NeoEngine
