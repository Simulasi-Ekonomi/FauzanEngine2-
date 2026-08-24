#include <cassert>
#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

namespace NeoEngine
{

class GLTFAccessor
{
public:

    static std::vector<float> ReadFloatArray(
        const uint8_t* buffer,
        size_t offset,
        size_t count,
        size_t stride
    );

};

}
