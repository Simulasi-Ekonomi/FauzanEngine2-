#pragma once
#include <cstdint>
#include <cstdlib>
#include <android/log.h>

namespace NeoEngine {

class StackAllocator {
public:
    explicit StackAllocator(size_t size) : m_size(size) {
        // Gunakan aligned_alloc untuk base pointer agar lebih gahar di mobile
        m_base = static_cast<uint8_t*>(malloc(size));
        m_current = m_base;
        if (!m_base) __android_log_assert("FAIL", "StackAlloc", "Out of Physical RAM");
    }

    ~StackAllocator() { free(m_base); }

    template<typename T, typename... Args>
    T* New(Args&&... args) {
        void* ptr = Allocate(sizeof(T), alignof(T));
        return ptr ? new(ptr) T(std::forward<Args>(args)...) : nullptr;
    }

    void* Allocate(size_t size, size_t alignment) {
        uintptr_t curr = reinterpret_cast<uintptr_t>(m_current);
        uintptr_t mask = alignment - 1;
        uintptr_t aligned = (curr + mask) & ~mask;
        
        if (aligned + size > reinterpret_cast<uintptr_t>(m_base) + m_size) {
            return nullptr; // Stack Overflow!
        }
        
        m_current = reinterpret_cast<uint8_t*>(aligned + size);
        return reinterpret_cast<void*>(aligned);
    }

    void Reset() { m_current = m_base; }

private:
    uint8_t* m_base;
    uint8_t* m_current;
    size_t m_size;
};

} // namespace
