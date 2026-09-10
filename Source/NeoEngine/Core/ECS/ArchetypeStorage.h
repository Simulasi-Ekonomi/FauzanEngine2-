#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "ArchetypeChunk.h"

namespace NeoEngine {

class ArchetypeStorage {
private:
    std::unordered_map<uint64_t, std::vector<ArchetypeChunk>> archetypes;

public:
    ArchetypeChunk& GetOrCreate(uint64_t signatureValue)
    {
        auto& chunks = archetypes[signatureValue];
        if (chunks.empty() || !chunks.back().HasSpace()) {
            chunks.emplace_back(Signature(signatureValue));
        }
        return chunks.back();
    }

    [[nodiscard]] std::unordered_map<uint64_t, std::vector<ArchetypeChunk>>& GetAll() noexcept
    {
        return archetypes;
    }

    [[nodiscard]] const std::unordered_map<uint64_t, std::vector<ArchetypeChunk>>& GetAll() const noexcept
    {
        return archetypes;
    }
};

} // namespace NeoEngine
