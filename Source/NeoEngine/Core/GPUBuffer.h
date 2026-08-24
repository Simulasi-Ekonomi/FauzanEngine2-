#pragma once
#include <cstdint>

namespace NeoEngine {

struct GPUBuffer {
    uint32_t id = 0;
    size_t size = 0;
    GPUBuffer() = default;
};

} // namespace NeoEngine
