#pragma once
#include <cstdlib>
#include <new>

namespace NeoEngine {
    class EngineAllocator {
    public:
        static void* Allocate(size_t size) {
            void* p = std::malloc(size);
            if (!p) throw std::bad_alloc();
            return p;
        }
        static void Deallocate(void* p) noexcept { std::free(p); }
        static void* Reallocate(void* p, size_t newSize) {
            void* np = std::realloc(p, newSize);
            if (!np) throw std::bad_alloc();
            return np;
        }
    };
}
