#pragma once
#include <vector>
#include <unordered_map>
#include <cstdint>

#include "Entity.h"
#include "Signature.h"
#include "ArchetypeChunk.h"

namespace NeoEngine {

struct Archetype {

    Signature signature;

    std::vector<Entity> entities;

    std::unordered_map<size_t, std::vector<uint8_t>> componentArrays;

    size_t entityCount = 0;

    std::vector<ArchetypeChunk> chunks;

    Archetype(const Signature& sig)
        : signature(sig)
    {}

    ArchetypeChunk& GetOrCreateChunk()
    {
        for(auto& chunk : chunks)
        {
            if(chunk.HasSpace())
                return chunk;
        }

        chunks.emplace_back();
        return chunks.back();
    }

};

}