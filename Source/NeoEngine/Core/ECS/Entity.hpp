#pragma once
#include <cstdint>

namespace NeoEngine {
    using Entity = uint32_t;
    constexpr Entity MAX_ENTITIES = 1000000;
    constexpr Entity INVALID_ENTITY = 0;
}
