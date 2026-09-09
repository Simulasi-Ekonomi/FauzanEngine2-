#pragma once
#include "Components.h"
#include <deque>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace NeoEngine {

using EntityID = uint32_t;

constexpr uint32_t COMP_POSITION = 1 << 0;
constexpr uint32_t COMP_VELOCITY = 1 << 1;
constexpr uint32_t COMP_COLLIDER = 1 << 2;
constexpr uint32_t COMP_MESH     = 1 << 3;
constexpr uint32_t COMP_ROTATION = 1 << 4;

struct ArchetypeChunk {
    uint32_t componentMask = 0;
    size_t count = 0;
    size_t capacity = 0;

    EntityID* entities = nullptr;
    float* posX = nullptr;
    float* posY = nullptr;
    float* posZ = nullptr;
    float* velX = nullptr;
    float* velY = nullptr;
    float* velZ = nullptr;
    float* radius = nullptr;
    float* invMass = nullptr;
    uint32_t* meshID = nullptr;
    float* rotX = nullptr;
    float* rotY = nullptr;
    float* rotZ = nullptr;
};

class ArchetypeManager {
public:
    ArchetypeManager();
    ~ArchetypeManager();

    EntityID CreateEntity(uint32_t componentMask);
    void DestroyEntity(EntityID id);

    void SetPosX(EntityID id, float v) { SetComponentValue(id, v, &ArchetypeChunk::posX); }
    void SetPosZ(EntityID id, float v) { SetComponentValue(id, v, &ArchetypeChunk::posZ); }
    void SetVelX(EntityID id, float v) { SetComponentValue(id, v, &ArchetypeChunk::velX); }
    void SetVelZ(EntityID id, float v) { SetComponentValue(id, v, &ArchetypeChunk::velZ); }
    void SetRadius(EntityID id, float v) { SetComponentValue(id, v, &ArchetypeChunk::radius); }
    void SetInvMass(EntityID id, float v) { SetComponentValue(id, v, &ArchetypeChunk::invMass); }
    [[nodiscard]] bool HasEntity(EntityID id) const { return entityToChunk_.contains(id); }
    [[nodiscard]] bool HasPosition(EntityID id) const {
        const auto it = entityToChunk_.find(id);
        return it != entityToChunk_.end() && it->second != nullptr && it->second->posX != nullptr;
    }
    void MarkPhysicsDirty() { ++physicsRevision_; }
    uint64_t GetPhysicsRevision() const { return physicsRevision_; }

    template<typename... Args>
    std::vector<ArchetypeChunk*> GetChunks();

private:
    template<typename T>
    void SetComponentValue(EntityID id, T value, T* ArchetypeChunk::*member) {
        const auto chunkIt = entityToChunk_.find(id);
        const auto indexIt = entityToIndex_.find(id);
        if (chunkIt == entityToChunk_.end() || indexIt == entityToIndex_.end()) return;
        ArchetypeChunk* chunk = chunkIt->second;
        const size_t index = indexIt->second;
        T* data = chunk ? chunk->*member : nullptr;
        if (!data || index >= chunk->count) return;
        data[index] = value;
        MarkPhysicsDirty();
    }

    ArchetypeChunk* FindOrCreateChunk(uint32_t componentMask);

    std::deque<ArchetypeChunk> chunks_;
    EntityID nextEntityID_ = 0;
    std::unordered_map<EntityID, ArchetypeChunk*> entityToChunk_;
    std::unordered_map<EntityID, size_t> entityToIndex_;
    uint64_t physicsRevision_ = 1;
};

} // namespace NeoEngine
