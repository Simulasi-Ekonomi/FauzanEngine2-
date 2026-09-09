#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace Neo {
    class Arena {
    private:
        std::uint8_t* memory = nullptr;
        std::size_t capacity = 0;
        std::size_t offset = 0;

        static bool IsPowerOfTwo(std::size_t value) {
            return value != 0 && (value & (value - 1)) == 0;
        }

    public:
        explicit Arena(std::size_t size) : capacity(size) {
            if (size != 0) {
                memory = static_cast<std::uint8_t*>(std::malloc(size));
                if (memory == nullptr) capacity = 0;
            }
        }

        ~Arena() { std::free(memory); }

        Arena(const Arena&) = delete;
        Arena& operator=(const Arena&) = delete;

        Arena(Arena&& other) noexcept
            : memory(other.memory), capacity(other.capacity), offset(other.offset) {
            other.memory = nullptr;
            other.capacity = 0;
            other.offset = 0;
        }

        Arena& operator=(Arena&& other) noexcept {
            if (this == &other) return *this;
            std::free(memory);
            memory = other.memory;
            capacity = other.capacity;
            offset = other.offset;
            other.memory = nullptr;
            other.capacity = 0;
            other.offset = 0;
            return *this;
        }

        void* Alloc(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) {
            if (memory == nullptr || size == 0 || !IsPowerOfTwo(alignment)) return nullptr;

            const std::uintptr_t currentAddress =
                reinterpret_cast<std::uintptr_t>(memory) + offset;
            const std::size_t padding =
                static_cast<std::size_t>(-currentAddress) & (alignment - 1);

            if (padding > capacity - offset) return nullptr;
            const std::size_t available = capacity - offset - padding;
            if (size > available) return nullptr;

            std::uint8_t* ptr = memory + offset + padding;
            offset += padding + size;
            return ptr;
        }

        void Reset() { offset = 0; }

        [[nodiscard]] std::size_t GetUsedMemory() const { return offset; }
        [[nodiscard]] std::size_t GetCapacity() const { return capacity; }
    };
}
