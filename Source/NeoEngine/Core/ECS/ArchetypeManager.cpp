#include "ArchetypeManager.h"
#include <cstring>
#include <algorithm>

namespace NeoEngine {

ArchetypeManager::ArchetypeManager() : nextEntityID_(0) {
    chunks_.reserve(256);
}

ArchetypeManager::~ArchetypeManager() {
    for (auto& chunk : chunks_) {
        delete[] chunk.entities;
        delete[] chunk.posX; delete[] chunk.posY; delete[] chunk.posZ;
        delete[] chunk.velX; delete[] chunk.velY; delete[] chunk.velZ;
        delete[] chunk.radius; delete[] chunk.invMass;
        delete[] chunk.meshID;
        delete[] chunk.rotX; delete[] chunk.rotY; delete[] chunk.rotZ;
        chunk = {};
    }
}

ArchetypeChunk* ArchetypeManager::FindOrCreateChunk(uint32_t componentMask) {
    for (auto& chunk : chunks_) {
        if (chunk.componentMask == componentMask && chunk.count < chunk.capacity) {
            return &chunk;
        }
    }
    
    ArchetypeChunk newChunk;
    newChunk.componentMask = componentMask;
    newChunk.count = 0;
    newChunk.capacity = 1024;
    
    newChunk.entities = new EntityID[newChunk.capacity]();
    if (componentMask & COMP_POSITION) {
        newChunk.posX = new float[newChunk.capacity]();
        newChunk.posY = new float[newChunk.capacity]();
        newChunk.posZ = new float[newChunk.capacity]();
    }
    if (componentMask & COMP_VELOCITY) {
        newChunk.velX = new float[newChunk.capacity]();
        newChunk.velY = new float[newChunk.capacity]();
        newChunk.velZ = new float[newChunk.capacity]();
    }
    if (componentMask & COMP_COLLIDER) {
        newChunk.radius = new float[newChunk.capacity]();
        newChunk.invMass = new float[newChunk.capacity]();
    }
    if (componentMask & COMP_MESH) {
        newChunk.meshID = new uint32_t[newChunk.capacity]();
    }
    if (componentMask & COMP_ROTATION) {
        newChunk.rotX = new float[newChunk.capacity]();
        newChunk.rotY = new float[newChunk.capacity]();
        newChunk.rotZ = new float[newChunk.capacity]();
    }
    
    chunks_.push_back(newChunk);
    return &chunks_.back();
}

EntityID ArchetypeManager::CreateEntity(uint32_t componentMask) {
    auto* chunk = FindOrCreateChunk(componentMask);
    size_t idx = chunk->count;
    
    // 🔴 PENTING: gunakan ID unik global
    EntityID id = nextEntityID_++;
    
    chunk->entities[idx] = id;
    
    if (componentMask & COMP_POSITION) {
        chunk->posX[idx] = 0.0f;
        chunk->posY[idx] = 0.0f;
        chunk->posZ[idx] = 0.0f;
    }
    if (componentMask & COMP_VELOCITY) {
        chunk->velX[idx] = 0.0f;
        chunk->velY[idx] = 0.0f;
        chunk->velZ[idx] = 0.0f;
    }
    if (componentMask & COMP_COLLIDER) {
        chunk->radius[idx] = 1.0f;
        chunk->invMass[idx] = 1.0f;
    }
    if (componentMask & COMP_MESH) {
        chunk->meshID[idx] = 0;
    }
    if (componentMask & COMP_ROTATION) {
        chunk->rotX[idx] = 0.0f;
        chunk->rotY[idx] = 0.0f;
        chunk->rotZ[idx] = 0.0f;
    }
    
    chunk->count++;
    entityToChunk_[id] = chunk;
    entityToIndex_[id] = idx;
    MarkPhysicsDirty();
    return id;
}

void ArchetypeManager::DestroyEntity(EntityID id) {
    const auto chunkIt = entityToChunk_.find(id);
    const auto indexIt = entityToIndex_.find(id);
    if (chunkIt == entityToChunk_.end() || indexIt == entityToIndex_.end()) return;
    ArchetypeChunk* chunk = chunkIt->second;
    if (!chunk || chunk->count == 0) return;
    const size_t index = indexIt->second;
    const size_t last = chunk->count - 1;
    if (index >= chunk->count) return;
    if (index != last) {
        const EntityID moved = chunk->entities[last];
        chunk->entities[index] = moved;
        if (chunk->componentMask & COMP_POSITION) {
            chunk->posX[index] = chunk->posX[last]; chunk->posY[index] = chunk->posY[last]; chunk->posZ[index] = chunk->posZ[last];
        }
        if (chunk->componentMask & COMP_VELOCITY) {
            chunk->velX[index] = chunk->velX[last]; chunk->velY[index] = chunk->velY[last]; chunk->velZ[index] = chunk->velZ[last];
        }
        if (chunk->componentMask & COMP_COLLIDER) {
            chunk->radius[index] = chunk->radius[last]; chunk->invMass[index] = chunk->invMass[last];
        }
        if (chunk->componentMask & COMP_MESH) chunk->meshID[index] = chunk->meshID[last];
        if (chunk->componentMask & COMP_ROTATION) {
            chunk->rotX[index] = chunk->rotX[last]; chunk->rotY[index] = chunk->rotY[last]; chunk->rotZ[index] = chunk->rotZ[last];
        }
        entityToIndex_[moved] = index;
    }
    --chunk->count;
    entityToChunk_.erase(chunkIt);
    entityToIndex_.erase(indexIt);
    MarkPhysicsDirty();
}

template<typename... Args>
std::vector<ArchetypeChunk*> ArchetypeManager::GetChunks() {
    std::vector<ArchetypeChunk*> result;
    uint32_t requiredMask = 0;
    ((requiredMask |= (std::is_same_v<Args, PositionComponent> ? COMP_POSITION : 0) |
                      (std::is_same_v<Args, VelocityComponent> ? COMP_VELOCITY : 0) |
                      (std::is_same_v<Args, ColliderComponent> ? COMP_COLLIDER : 0) |
                      (std::is_same_v<Args, MeshComponent> ? COMP_MESH : 0) |
                      (std::is_same_v<Args, RotationComponent> ? COMP_ROTATION : 0)), ...);
    
    for (auto& chunk : chunks_) {
        if ((chunk.componentMask & requiredMask) == requiredMask && chunk.count > 0) {
            result.push_back(&chunk);
        }
    }
    return result;
}

template std::vector<ArchetypeChunk*> ArchetypeManager::GetChunks<PositionComponent, VelocityComponent, ColliderComponent>();

} // namespace NeoEngine
