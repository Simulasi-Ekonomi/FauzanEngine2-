#include "GLTFAccessor.h"

#include <cstring>
#include <limits>

namespace NeoEngine {

std::vector<float> GLTFAccessor::ReadFloatArray(
    const uint8_t* buffer,
    size_t offset,
    size_t count,
    size_t stride)
{
    std::vector<float> result;
    if (buffer == nullptr || count == 0U || stride < sizeof(float)) {
        return result;
    }
    if (offset > std::numeric_limits<size_t>::max() - (count - 1U) * stride) {
        return result;
    }

    result.reserve(count);
    for (size_t i = 0U; i < count; ++i) {
        float value = 0.0F;
        std::memcpy(&value, buffer + offset + i * stride, sizeof(value));
        result.push_back(value);
    }
    return result;
}

} // namespace NeoEngine
