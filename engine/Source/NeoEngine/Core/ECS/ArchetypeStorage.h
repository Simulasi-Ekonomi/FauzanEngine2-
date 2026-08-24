#pragma once

#include <unordered_map>
#include <vector>
#include "ArchetypeChunk.h"

namespace NeoEngine {

class ArchetypeStorage {

private:

    std::unordered_map<uint64_t, std::vector<ArchetypeChunk>> archetypes;

public:

    ArchetypeChunk& GetOrCreate(uint64_t signature)
    {
        auto& vec = archetypes[signature];

        if(vec.empty() || vec.back().entityCount >= CHUNK_SIZE)
        {
            vec.emplace_back(Signature(signature));
        }

        return vec.back();
    }

    std::unordered_map<uint64_t, std::vector<ArchetypeChunk>>& GetAll()
    {
        return archetypes;
    }

};

}
