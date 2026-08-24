#pragma once
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <stdexcept>

namespace NeoEngine {

class MemoryBlock {
public:
    explicit MemoryBlock(size_t elementSize, size_t capacity)
        : m_elementSize(elementSize), m_capacity(capacity) {
        m_buffer = static_cast<uint8_t*>(malloc(elementSize * capacity));
        if (!m_buffer) throw std::bad_alloc();
    }

    ~MemoryBlock() { free(m_buffer); }

    void* Get(size_t index) {
        if (index >= m_capacity) return nullptr;
        return m_buffer + index * m_elementSize;
    }

    const void* Get(size_t index) const {
        if (index >= m_capacity) return nullptr;
        return m_buffer + index * m_elementSize;
    }

    size_t GetCapacity() const { return m_capacity; }
    size_t GetElementSize() const { return m_elementSize; }

private:
    uint8_t* m_buffer = nullptr;
    size_t m_elementSize = 0;
    size_t m_capacity = 0;
};

} // namespace NeoEngine
