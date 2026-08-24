#pragma once

#include <immintrin.h>
#include "Registry.h"
#include "Component.h"

class SIMDMovementSystem : public System
{

public:

    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override
    {

        auto* posPool = registry.GetPool<Position>();
        auto* velPool = registry.GetPool<Velocity>();

        uint32_t count = posPool->Size();

        for (uint32_t i = 0; i < count; i += 4)
        {

            __m128 px = _mm_set_ps(
                posPool->GetEntityAtDenseIndex(i+3),
                posPool->GetEntityAtDenseIndex(i+2),
                posPool->GetEntityAtDenseIndex(i+1),
                posPool->GetEntityAtDenseIndex(i)
            );

            __m128 vx = _mm_set_ps(
                velPool->GetEntityAtDenseIndex(i+3),
                velPool->GetEntityAtDenseIndex(i+2),
                velPool->GetEntityAtDenseIndex(i+1),
                velPool->GetEntityAtDenseIndex(i)
            );

            __m128 delta = _mm_mul_ps(vx, _mm_set1_ps(dt));

            px = _mm_add_ps(px, delta);

        }

    }

};
