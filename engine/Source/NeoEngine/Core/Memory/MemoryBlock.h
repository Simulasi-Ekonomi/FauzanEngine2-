#pragma once
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <stdexcept>

class MemoryBlock {
private:
    uint8_t* buffer = nullptr;
    size_t capacity = 0;
    size_t elementSize = 0;
    size_t alignment = alignof(std::max_align_t);

public:
    MemoryBlock(size_t elementSize, size_t capacity)
        : capacity(capacity), elementSize(elementSize)
    {
        size_t totalSize = elementSize * capacity;

        void* ptr = nullptr;
        if (posix_memalign(&ptr, alignment, totalSize) != 0) {
            throw std::bad_alloc();
        }

        buffer = static_cast<uint8_t*>(ptr);
    }

    ~MemoryBlock() {
        free(buffer);
    }

    void* Get(size_t index) {
        return buffer + index * elementSize;
    }
};
