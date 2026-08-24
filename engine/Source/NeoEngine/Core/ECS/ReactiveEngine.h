#pragma once

#include "Registry.h"
#include <vector>
#include <algorithm>

class ChunkedReactiveSystem : public System
{

protected:

    struct Chunk
    {
        [[maybe_unused]] std::vector<Entity> entities;
    };

    [[maybe_unused]] std::vector<Chunk> chunks;
    Signature requiredSignature;

public:

    ChunkedReactiveSystem(const Signature& sig)
        : requiredSignature(sig)
    {}

    void UpdateEntityMembership(Registry& registry, Entity e) override
    {

        bool match = (registry.GetSignature(e) & requiredSignature) == requiredSignature;

        for (auto& chunk : chunks)
        {
            auto it = std::find(chunk.entities.begin(), chunk.entities.end(), e);

            if (it != chunk.entities.end())
            {
                if (!match)
                    chunk.entities.erase(it);

                return;
            }
        }

        if (match)
        {
            if (chunks.empty() || chunks.back().entities.size() >= 256)
                chunks.emplace_back();

            chunks.back().entities.push_back(e);
        }
    }

    template<typename Func>
    void ForEach(Registry& registry, Func fn)
    {
        for (auto& chunk : chunks)
        {
            for (auto& e : chunk.entities)
            {
                if (registry.IsAlive(e))
                    fn(e);
            }
        }
    }

};
