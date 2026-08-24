#pragma once
#include <cstddef>
#include <cstdlib>
#include <cstdint>

namespace Neo {
    class Arena {
    private:
        uint8_t* memory;
        size_t capacity;
        size_t offset;

    public:
        Arena(size_t size) : capacity(size), offset(0) {
            memory = (uint8_t*)std::malloc(size);
        }

        ~Arena() {
            std::free(memory);
        }

        // Alokasi O(1) - Sangat cepat!
        void* Alloc(size_t size, size_t alignment = 8) {
            // Hitung alignment padding
            size_t currentAddr = (size_t)memory + offset;
            size_t padding = (alignment - (currentAddr % alignment)) % alignment;

            if (offset + padding + size > capacity) return nullptr;

            uint8_t* ptr = memory + offset + padding;
            offset += padding + size;
            return (void*)ptr;
        }

        void Reset() {
            offset = 0;
        }

        size_t GetUsedMemory() const { return offset; }
    };
}
