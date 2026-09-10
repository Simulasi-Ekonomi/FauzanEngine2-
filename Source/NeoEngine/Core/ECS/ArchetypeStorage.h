#pragma once

#include <cstdint>
#include <deque>
#include <unordered_map>

#include "ArchetypeChunk.h"

namespace NeoEngine {

class ArchetypeStorage {
private:
    // deque keeps references to existing chunks stable when another chunk is added.
    std::unordered_map<uint64_t, std::deque<ArchetypeChunk>> archetypes;

public:
    ArchetypeChunk& GetOrCreate(uint64_t signatureValue)
    {
        auto& chunks = archetypes[signatureValue];
        if (chunks.empty() || !chunks.back().HasSpace()) {
            chunks.emplace_back(Signature(signatureValue));
        }
        return chunks.back();
    }

    [[nodiscard]] std::unordered_map<uint64_t, std::deque<ArchetypeChunk>>& GetAll() noexcept
    {
        return archetypes;
    }

    [[nodiscard]] const std::unordered_map<uint64_t, std::deque<ArchetypeChunk>>& GetAll() const noexcept
    {
        return archetypes;
    }
};

} // namespace NeoEngine
