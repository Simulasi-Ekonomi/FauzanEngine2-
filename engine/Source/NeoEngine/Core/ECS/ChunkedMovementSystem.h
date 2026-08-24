#pragma once

#include "ReactiveSystem.h"
#include "Registry.h"
#include "Core/Scheduler/ParallelFor.h"

class ChunkedMovementSystem : public ReactiveSystem
{
public:

    ChunkedMovementSystem()
        : ReactiveSystem(
            Signature()
            .set(GetComponentTypeID<Position>())
            .set(GetComponentTypeID<Velocity>())
        )
    {}

    void Update(float dt, RegistryUpdate(Registry& registry, float dt) registry) override
    {
        auto& chunks =
            registry.GetChunks<Position, Velocity>();

        const size_t totalChunks = chunks.size();

        NeoEngine::ParallelFor(
            0,
            totalChunks,
            1,
            [&](size_t start, size_t end)
        {
            for(size_t c = start; c < end; c++)
            {
                auto& chunk = chunks[c];

                auto* posArr = chunk.GetArray<Position>();
                auto* velArr = chunk.GetArray<Velocity>();

                size_t count = chunk.count;

                for(size_t i = 0; i < count; i++)
                {
                    posArr[i].x += velArr[i].x * dt;
                    posArr[i].y += velArr[i].y * dt;
                }
            }
        });
    }
};