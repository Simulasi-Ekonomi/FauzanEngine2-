#include <cassert>
#include "GPUMemoryBudget.h"

namespace NeoEngine
{

size_t GPUMemoryBudget::budget = 0;
size_t GPUMemoryBudget::used = 0;
std::mutex GPUMemoryBudget::lock;

void GPUMemoryBudget::Initialize(size_t b)
{
    budget = b;
    used = 0;
}

bool GPUMemoryBudget::Request(size_t bytes)
{
    std::lock_guard<std::mutex> guard(lock);

    if(used + bytes > budget)
        return false;

    used += bytes;
    return true;
}

void GPUMemoryBudget::Release(size_t bytes)
{
    std::lock_guard<std::mutex> guard(lock);

    if(bytes <= used)
        used -= bytes;
}

size_t GPUMemoryBudget::Used()
{
    return used;
}

size_t GPUMemoryBudget::Budget()
{
    return budget;
}

}
