#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>
#include "Entity.h"
#include "Core/ECS/Registry.h"

namespace NeoEngine {

struct ArchetypeChunk {
    std::vector<uint32_t> entities;
    static constexpr size_t MAX_ENTITIES = 512;
};

template<typename... ComponentTypes>
class ChunkedFullEngine {
private:
    std::vector<ArchetypeChunk*> chunks;

public:
    ~ChunkedFullEngine() {
        for (auto* chunk : chunks) delete chunk;
    }

    void UpdateEntityMembership(Registry& registry, Entity e) {
        bool match = (registry.HasComponent<ComponentTypes>(e) && ...);
        for (auto* chunk : chunks) {
            auto it = std::find(chunk->entities.begin(), chunk->entities.end(), e.index);
            if (it != chunk->entities.end() && !match) {
                chunk->entities.erase(it);
                return;
            }
        }
        if (match) {
            if (chunks.empty() || chunks.back()->entities.size() >= ArchetypeChunk::MAX_ENTITIES) {
                chunks.push_back(new ArchetypeChunk());
            }
            chunks.back()->entities.push_back(e.index);
        }
    }

    template<typename Func>
    void Each(Registry& registry, Func func) {
        for (auto* chunk : chunks) {
            for (uint32_t idx : chunk->entities) {
                func(Entity{idx});
            }
        }
    }

    size_t GetChunkCount() const { return chunks.size(); }
    size_t GetEntityCount() const {
        size_t count = 0;
        for (auto* chunk : chunks) count += chunk->entities.size();
        return count;
    }
};

} // namespace NeoEngine
