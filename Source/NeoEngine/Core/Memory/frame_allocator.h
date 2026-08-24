#pragma once
#include <cstdint>
#include <cstdlib>

namespace NeoEngine {
    class FrameAllocator_Legacy {
    public:
        FrameAllocator_Legacy(size_t size) : capacity(size) {
            memory = (uint8_t*)std::malloc(size);
            offset = 0;
        }
        ~FrameAllocator_Legacy() { std::free(memory); }
        void* allocate(size_t size, size_t align = 16) {
            size_t current = (size_t)(memory + offset);
            size_t aligned = (current + align - 1) & ~(align - 1);
            size_t newOffset = aligned - (size_t)memory + size;
            if (newOffset > capacity) return nullptr;
            offset = newOffset;
            return (void*)aligned;
        }
        void reset() { offset = 0; }
    private:
        uint8_t* memory;
        size_t capacity;
        size_t offset;
    };
}
