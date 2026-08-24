#include "MemoryTracker.h"
#include <iostream>
#include <fstream>

namespace NeoEngine
{

std::unordered_map<void*, MemoryTracker::Allocation> MemoryTracker::allocations;
std::mutex MemoryTracker::mutex;

void* MemoryTracker::Allocate(size_t size, const char* file, int line)
{
    void* ptr = malloc(size);

    {
        std::lock_guard<std::mutex> lock(mutex);
        allocations[ptr] = {size, file, line};
    }

    return ptr;
}

void MemoryTracker::Deallocate(void* ptr)
{
    if (!ptr)
        return;

    {
        std::lock_guard<std::mutex> lock(mutex);
        allocations.erase(ptr);
    }

    free(ptr);
}

void MemoryTracker::Report()
{
    std::lock_guard<std::mutex> lock(mutex);  // FIX: lock saat baca map

    std::ofstream report("neoengine_memory_report.txt");

    size_t total = 0;

    for (const auto& it : allocations)  // FIX: const& karena read-only
    {
        report << "Leak: "
               << it.second.size
               << " bytes | "
               << it.second.file
               << ":"
               << it.second.line
               << "\n";  // FIX: "\n" lebih efisien dari std::endl (no flush)

        total += it.second.size;
    }

    report << "======================\n";
    report << "Total leaked: " << total << " bytes\n";

    if (total > 0)
    {
        std::cerr << "Memory leaks detected: " << total << " bytes\n";
    }
}

} // namespace NeoEngine
