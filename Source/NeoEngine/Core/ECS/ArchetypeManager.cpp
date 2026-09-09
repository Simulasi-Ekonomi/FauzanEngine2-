#include "ArchetypeManager.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <type_traits>

namespace NeoEngine {

ArchetypeManager::ArchetypeManager() : nextEntityID_(0) {}

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

    constexpr size_t capacity = 1024;
    ArchetypeChunk newChunk;
    newChunk.componentMask = componentMask;
    newChunk.count = 0;
    newChunk.capacity = capacity;

    auto entities = std::make_unique<EntityID[]>(capacity);
    std::unique_ptr<float[]> posX;
    std::unique_ptr<float[]> posY;
    std::unique_ptr<float[]> posZ;
    std::unique_ptr<float[]> velX;
    std::unique_ptr<float[]> velY;
    std::unique_ptr<float[]> velZ;
    std::unique_ptr<float[]> radius;
    std::unique_ptr<float[]> invMass;
    std::unique_ptr<uint32_t[]> meshID;
    std::unique_ptr<float[]> rotX;
    std::unique_ptr<float[]> rotY;
    std::unique_ptr<float[]> rotZ;

    if (componentMask & COMP_POSITION) {
        posX = std::make_unique<float[]>(capacity);
        posY = std::make_unique<float[]>(capacity);
        posZ = std::make_unique<float[]>(capacity);
    }
    if (componentMask & COMP_VELOCITY) {
        velX = std::make_unique<float[]>(capacity);
        velY = std::make_unique<float[]>(capacity);
        velZ = std::make_unique<float[]>(capacity);
    }
    if (componentMask & COMP_COLLIDER) {
        radius = std::make_unique<float[]>(capacity);
        invMass = std::make_unique<float[]>(capacity);
    }
    if (componentMask & COMP_MESH) {
        meshID = std::make_unique<uint32_t[]>(capacity);
    }
    if (componentMask & COMP_ROTATION) {
        rotX = std::make_unique<float[]>(capacity);
        rotY = std::make_unique<float[]>(capacity);
        rotZ = std::make_unique<float[]>(capacity);
    }

    newChunk.entities = entities.release();
    newChunk.posX = posX.release();
    newChunk.posY = posY.release();
    newChunk.posZ = posZ.release();
    newChunk.velX = velX.release();
    newChunk.velY = velY.release();
    newChunk.velZ = velZ.release();
    newChunk.radius = radius.release();
    newChunk.invMass = invMass.release();
    newChunk.meshID = meshID.release();
    newChunk.rotX = rotX.release();
    newChunk.rotY = rotY.release();
    newChunk.rotZ = rotZ.release();

    try {
        chunks_.push_back(newChunk);
    } catch (...) {
        delete[] newChunk.entities;
        delete[] newChunk.posX; delete[] newChunk.posY; delete[] newChunk.posZ;
        delete[] newChunk.velX; delete[] newChunk.velY; delete[] newChunk.velZ;
        delete[] newChunk.radius; delete[] newChunk.invMass;
        delete[] newChunk.meshID;
        delete[] newChunk.rotX; delete[] newChunk.rotY; delete[] newChunk.rotZ;
        throw;
    }

    return &chunks_.back();
}

EntityID ArchetypeManager::CreateEntity(uint32_t componentMask) {
    auto* chunk = FindOrCreateChunk(componentMask);
    if (!chunk) return std::numeric_limits<EntityID>::max();

    const size_t idx = chunk->count;
    if (idx >= chunk->capacity || nextEntityID_ == std::numeric_limits<EntityID>::max()) {
        return std::numeric_limits<EntityID>::max();
    }

    const EntityID id = nextEntityID_++;
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

    try {
        entityToChunk_.emplace(id, chunk);
        try {
            entityToIndex_.emplace(id, idx);
        } catch (...) {
            entityToChunk_.erase(id);
            throw;
        }
    } catch (...) {
        --nextEntityID_;
        chunk->entities[idx] = 0;
        throw;
    }

    ++chunk->count;
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
            chunk->posX[index] = chunk->posX[last];
            chunk->posY[index] = chunk->posY[last];
            chunk->posZ[index] = chunk->posZ[last];
        }
        if (chunk->componentMask & COMP_VELOCITY) {
            chunk->velX[index] = chunk->velX[last];
            chunk->velY[index] = chunk->velY[last];
            chunk->velZ[index] = chunk->velZ[last];
        }
        if (chunk->componentMask & COMP_COLLIDER) {
            chunk->radius[index] = chunk->radius[last];
            chunk->invMass[index] = chunk->invMass[last];
        }
        if (chunk->componentMask & COMP_MESH) chunk->meshID[index] = chunk->meshID[last];
        if (chunk->componentMask & COMP_ROTATION) {
            chunk->rotX[index] = chunk->rotX[last];
            chunk->rotY[index] = chunk->rotY[last];
            chunk->rotZ[index] = chunk->rotZ[last];
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

template std::vector<ArchetypeChunk*> ArchetypeManager::GetChunks<PositionComponent>();
template std::vector<ArchetypeChunk*> ArchetypeManager::GetChunks<PositionComponent, VelocityComponent>();
template std::vector<ArchetypeChunk*> ArchetypeManager::GetChunks<PositionComponent, ColliderComponent>();

template std::vector<ArchetypeChunk*> ArchetypeManager::GetChunks<VelocityComponent>();
template std::vector<ArchetypeChunk*> ArchetypeManager::GetChunks<ColliderComponent>();
template std::vector<ArchetypeChunk*> ArchetypeManager::GetChunks<MeshComponent>();
template std::vector<ArchetypeChunk*> ArchetypeManager::GetChunks<RotationComponent>();

} // namespace NeoEngine
