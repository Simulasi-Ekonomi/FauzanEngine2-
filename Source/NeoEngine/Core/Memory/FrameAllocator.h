#pragma once
#include <cstddef>
#include <cstdint>

namespace NeoEngine {

class FrameAllocator {
public:
    explicit FrameAllocator(size_t capacity);
    ~FrameAllocator();
    void* Allocate(size_t size, size_t alignment = 8);
    void Reset();
private:
    uint8_t* memory_;
    size_t capacity_;
    size_t offset_ = 0;
};

} // namespace
