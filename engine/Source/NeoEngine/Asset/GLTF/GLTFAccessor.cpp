#include <cassert>
#include "GLTFAccessor.h"

namespace NeoEngine
{

std::vector<float> GLTFAccessor::ReadFloatArray(
    const uint8_t* buffer,
    size_t offset,
    size_t count,
    size_t stride
)
{
    std::vector<float> result;
    result.reserve(count);

    const uint8_t* base = buffer + offset;

    for(size_t i = 0; i < count; i++)
    {
        const float* value =
            reinterpret_cast<const float*>(base + i * stride);

        result.push_back(*value);
    }

    return result;
}

}
