#pragma once
#include <cstdint>
#include <vector>

namespace NeoEngine {

class FrameAllocator {
public:
    FrameAllocator(size_t size) { buffer.resize(size); }
    void* Allocate(size_t sz) {
        if(offset + sz > buffer.size()) return nullptr;
        void* ptr = buffer.data() + offset;
        offset += sz;
        return ptr;
    }
    void Reset() { offset = 0; }

private:
    [[maybe_unused]] std::vector<uint8_t> buffer;
    size_t offset = 0;
};

} // namespace NeoEngine
