#pragma once

#include <cstddef>
#include <vector>

#include "Entity.h"
#include "Signature.h"

namespace NeoEngine {

// Standalone archetype-storage chunk. This type is intentionally separate from
// ArchetypeManager's runtime SoA chunk, which remains canonical for physics.
class ArchetypeChunk {
public:
    static constexpr std::size_t CHUNK_SIZE = 1024;

    explicit ArchetypeChunk(const Signature& sig = Signature{})
        : signature(sig)
    {
        entities.reserve(CHUNK_SIZE);
    }

    [[nodiscard]] bool HasSpace() const noexcept
    {
        return entities.size() < CHUNK_SIZE;
    }

    [[nodiscard]] std::size_t EntityCount() const noexcept
    {
        return entities.size();
    }

    [[nodiscard]] std::size_t Capacity() const noexcept
    {
        return CHUNK_SIZE;
    }

    [[nodiscard]] const Signature& GetSignature() const noexcept
    {
        return signature;
    }

    bool AddEntity(const Entity& entity)
    {
        if (!HasSpace()) return false;
        entities.push_back(entity);
        return true;
    }

    bool RemoveEntity(EntityID id)
    {
        for (auto it = entities.begin(); it != entities.end(); ++it) {
            if (it->GetID() == id) {
                entities.erase(it);
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] const std::vector<Entity>& GetEntities() const noexcept
    {
        return entities;
    }

    [[nodiscard]] std::vector<Entity>& GetEntities() noexcept
    {
        return entities;
    }

private:
    Signature signature;
    std::vector<Entity> entities;
};

} // namespace NeoEngine
