#pragma once
#include <cstdint>
#include <cstdlib>

namespace NeoEngine {
    class Arena {
    public:
        Arena(size_t size) : m_size(size) {
            m_base = (uint8_t*)std::malloc(size);
            m_current = m_base;
        }
        ~Arena() { std::free(m_base); }
        void* Allocate(size_t size, size_t alignment = 16) {
            uintptr_t curr = (uintptr_t)m_current;
            uintptr_t aligned = (curr + alignment - 1) & ~(alignment - 1);
            if (aligned + size > (uintptr_t)m_base + m_size) return nullptr;
            m_current = (uint8_t*)(aligned + size);
            return (void*)aligned;
        }
        void Reset() { m_current = m_base; }
    private:
        uint8_t* m_base;
        uint8_t* m_current;
        size_t m_size;
    };
}
