#pragma once
#include <cstdint>
#include <cstddef>

namespace NeoEngine {

class LinearAllocator {
public:
    explicit LinearAllocator(size_t size);
    ~LinearAllocator();
    void* Allocate(size_t size, size_t alignment = 8);
    void Reset();
    size_t GetUsed() const { return offset_; }
    size_t GetTotal() const { return totalSize_; }
private:
    uint8_t* memory_ = nullptr;
    size_t totalSize_ = 0;
    size_t offset_ = 0;
};

} // namespace
