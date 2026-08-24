#pragma once

#include <cstddef>
#include <mutex>

namespace NeoEngine
{

class GPUMemoryBudget
{
public:

    static void Initialize(size_t budgetBytes);

    static bool Request(size_t bytes);

    static void Release(size_t bytes);

    static size_t Used();

    static size_t Budget();

private:

    static size_t budget;
    static size_t used;
    static std::mutex lock;

};

}
