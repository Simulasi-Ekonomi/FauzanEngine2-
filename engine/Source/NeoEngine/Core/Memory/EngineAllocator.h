#pragma once
#include <cstdlib>
#include <new>

namespace Neo {
    class EngineAllocator {
    public:
        static void* Allocate(size_t size) {
            void* p = std::malloc(size);
            if(!p) throw std::bad_alloc();
            return p;
        }
        static void Deallocate(void* p) {
            std::free(p);
        }
    };
}
