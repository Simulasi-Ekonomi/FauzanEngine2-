#include "Runtime/AnimationSpriteTintBinding.h"

#include <cmath>
#include <cstdio>
#include <limits>

int main() {
    using namespace NeoEngine;
    AnimationSpriteTintBinding binding; uint32_t rgba=0x12345678U;
    if (binding.Resolve(0.0F, rgba) || binding.LastError()!=AnimationSpriteTintBindingError::NotInitialized || rgba!=0x12345678U) return 1;
    if (!binding.Initialize({0xFF000000U,0xFFFFFFFFU}) || !binding.Resolve(0.0F,rgba) || rgba!=0xFF000000U || !binding.Resolve(0.5F,rgba) || rgba!=0xFF808080U || !binding.Resolve(1.0F,rgba) || rgba!=0xFFFFFFFFU) return 1;
    const uint32_t preserved=rgba; if (binding.Resolve(std::numeric_limits<float>::quiet_NaN(),rgba) || binding.LastError()!=AnimationSpriteTintBindingError::InvalidSample || rgba!=preserved) return 1;
    std::printf("ANIMATION_SPRITE_TINT_BINDING_SMOKE_OK endpoints=1 midpoint=1 noTransformWrite=1\n"); return 0;
}
