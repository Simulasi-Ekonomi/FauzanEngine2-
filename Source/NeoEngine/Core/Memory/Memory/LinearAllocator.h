#pragma once
#include <cstdint>
#include <cstdlib>

class LinearAllocator
{
public:

    LinearAllocator(size_t size)
    {
        memory = malloc(size);
        capacity = size;
    }

    ~LinearAllocator()
    {
        free(memory);
    }

    void* Allocate(size_t size)
    {
        if(offset + size > capacity)
            return nullptr;

        void* ptr = (uint8_t*)memory + offset;
        offset += size;

        return ptr;
    }

    void Reset()
    {
        offset = 0;
    }

private:

    void* memory;
    size_t capacity;
    size_t offset{0};
};
