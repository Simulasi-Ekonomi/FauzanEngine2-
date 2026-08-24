#pragma once

#include <unordered_map>
#include <mutex>
#include <cstddef>

namespace NeoEngine
{

class MemoryTracker
{
public:

    static void* Allocate(size_t size, const char* file, int line);
    static void  Deallocate(void* ptr);

    static void Report();

private:

    struct Allocation
    {
        size_t size;
        const char* file;
        int line;
    };

    static std::unordered_map<void*, Allocation> allocations;
    static std::mutex mutex;
};

}

#define NE_NEW(size) NeoEngine::MemoryTracker::Allocate(size, __FILE__, __LINE__)
#define NE_DELETE(ptr) NeoEngine::MemoryTracker::Deallocate(ptr)
