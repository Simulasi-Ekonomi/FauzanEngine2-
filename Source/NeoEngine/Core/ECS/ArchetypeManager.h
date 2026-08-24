#pragma once
#include "Components.h"
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace NeoEngine {

using EntityID = uint32_t;

// Component masks
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
    
    // Setters untuk benchmark (ditambahkan)
    void SetPosX(EntityID id, float v) { auto* c = entityToChunk_[id]; if(c&&c->posX) { c->posX[entityToIndex_[id]] = v; MarkPhysicsDirty(); } }
    void SetPosZ(EntityID id, float v) { auto* c = entityToChunk_[id]; if(c&&c->posZ) { c->posZ[entityToIndex_[id]] = v; MarkPhysicsDirty(); } }
    void SetVelX(EntityID id, float v) { auto* c = entityToChunk_[id]; if(c&&c->velX) { c->velX[entityToIndex_[id]] = v; MarkPhysicsDirty(); } }
    void SetVelZ(EntityID id, float v) { auto* c = entityToChunk_[id]; if(c&&c->velZ) { c->velZ[entityToIndex_[id]] = v; MarkPhysicsDirty(); } }
    void SetRadius(EntityID id, float v) { auto* c = entityToChunk_[id]; if(c&&c->radius) { c->radius[entityToIndex_[id]] = v; MarkPhysicsDirty(); } }
    void SetInvMass(EntityID id, float v) { auto* c = entityToChunk_[id]; if(c&&c->invMass) { c->invMass[entityToIndex_[id]] = v; MarkPhysicsDirty(); } }
    void MarkPhysicsDirty() { ++physicsRevision_; }
    uint64_t GetPhysicsRevision() const { return physicsRevision_; }
    
    template<typename... Args>
    std::vector<ArchetypeChunk*> GetChunks();
    
private:
    ArchetypeChunk* FindOrCreateChunk(uint32_t componentMask);
    
    std::vector<ArchetypeChunk> chunks_;
    EntityID nextEntityID_ = 0;
    std::unordered_map<EntityID, ArchetypeChunk*> entityToChunk_;
    std::unordered_map<EntityID, size_t> entityToIndex_;
    uint64_t physicsRevision_ = 1;
};

} // namespace NeoEngine
