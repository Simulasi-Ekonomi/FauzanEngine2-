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

void ArchetypeManager::SetComponentMask(EntityID id, uint32_t componentMask) {
    const auto it = entityToChunk_.find(id);
    const auto idx = entityToIndex_.find(id);
    if (it == entityToChunk_.end() || idx == entityToIndex_.end() || it->second == nullptr) return;
    if (it->second->componentMask == componentMask) return;
    const size_t oldIndex = idx->second;
    ArchetypeChunk* oldChunk = it->second;
    ArchetypeChunk* newChunk = FindOrCreateChunk(componentMask);
    if (newChunk == nullptr) return;
    if (newChunk == oldChunk) return;
    if (newChunk->count >= newChunk->capacity) return;
    const size_t newIndex = newChunk->count++;
    newChunk->entities[newIndex] = id;
    if (newChunk->posX && oldChunk->posX) newChunk->posX[newIndex] = oldChunk->posX[oldIndex];
    if (newChunk->posY && oldChunk->posY) newChunk->posY[newIndex] = oldChunk->posY[oldIndex];
    if (newChunk->posZ && oldChunk->posZ) newChunk->posZ[newIndex] = oldChunk->posZ[oldIndex];
    if (newChunk->velX && oldChunk->velX) newChunk->velX[newIndex] = oldChunk->velX[oldIndex];
    if (newChunk->velY && oldChunk->velY) newChunk->velY[newIndex] = oldChunk->velY[oldIndex];
    if (newChunk->velZ && oldChunk->velZ) newChunk->velZ[newIndex] = oldChunk->velZ[oldIndex];
    if (newChunk->radius && oldChunk->radius) newChunk->radius[newIndex] = oldChunk->radius[oldIndex];
    if (newChunk->invMass && oldChunk->invMass) newChunk->invMass[newIndex] = oldChunk->invMass[oldIndex];
    if (newChunk->meshID && oldChunk->meshID) newChunk->meshID[newIndex] = oldChunk->meshID[oldIndex];
    if (newChunk->rotX && oldChunk->rotX) newChunk->rotX[newIndex] = oldChunk->rotX[oldIndex];
    if (newChunk->rotY && oldChunk->rotY) newChunk->rotY[newIndex] = oldChunk->rotY[oldIndex];
    if (newChunk->rotZ && oldChunk->rotZ) newChunk->rotZ[newIndex] = oldChunk->rotZ[oldIndex];
    const size_t last = oldChunk->count - 1U;
    if (oldIndex != last) {
        const EntityID moved = oldChunk->entities[last];
        oldChunk->entities[oldIndex] = moved;
        if (oldChunk->posX) oldChunk->posX[oldIndex] = oldChunk->posX[last];
        if (oldChunk->posY) oldChunk->posY[oldIndex] = oldChunk->posY[last];
        if (oldChunk->posZ) oldChunk->posZ[oldIndex] = oldChunk->posZ[last];
        if (oldChunk->velX) oldChunk->velX[oldIndex] = oldChunk->velX[last];
        if (oldChunk->velY) oldChunk->velY[oldIndex] = oldChunk->velY[last];
        if (oldChunk->velZ) oldChunk->velZ[oldIndex] = oldChunk->velZ[last];
        if (oldChunk->radius) oldChunk->radius[oldIndex] = oldChunk->radius[last];
        if (oldChunk->invMass) oldChunk->invMass[oldIndex] = oldChunk->invMass[last];
        if (oldChunk->meshID) oldChunk->meshID[oldIndex] = oldChunk->meshID[last];
        if (oldChunk->rotX) oldChunk->rotX[oldIndex] = oldChunk->rotX[last];
        if (oldChunk->rotY) oldChunk->rotY[oldIndex] = oldChunk->rotY[last];
        if (oldChunk->rotZ) oldChunk->rotZ[oldIndex] = oldChunk->rotZ[last];
        entityToIndex_[moved] = oldIndex;
    }
    oldChunk->count--;
    entityToChunk_[id] = newChunk;
    entityToIndex_[id] = newIndex;
    MarkPhysicsDirty();
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


bool ArchetypeManager::TryGetPosition(EntityID id, float& x, float& y, float& z) const {
    const auto chunkIt = entityToChunk_.find(id);
    const auto indexIt = entityToIndex_.find(id);
    if (chunkIt == entityToChunk_.end() || indexIt == entityToIndex_.end() || chunkIt->second == nullptr) return false;
    const ArchetypeChunk* chunk = chunkIt->second;
    const size_t index = indexIt->second;
    if (index >= chunk->count || chunk->posX == nullptr || chunk->posY == nullptr || chunk->posZ == nullptr) return false;
    x = chunk->posX[index]; y = chunk->posY[index]; z = chunk->posZ[index];
    return true;
}

bool ArchetypeManager::TryGetVelocity(EntityID id, float& x, float& y, float& z) const {
    const auto chunkIt = entityToChunk_.find(id);
    const auto indexIt = entityToIndex_.find(id);
    if (chunkIt == entityToChunk_.end() || indexIt == entityToIndex_.end() || chunkIt->second == nullptr) return false;
    const ArchetypeChunk* chunk = chunkIt->second;
    const size_t index = indexIt->second;
    if (index >= chunk->count || chunk->velX == nullptr || chunk->velY == nullptr || chunk->velZ == nullptr) return false;
    x = chunk->velX[index]; y = chunk->velY[index]; z = chunk->velZ[index];
    return true;
}

bool ArchetypeManager::TryGetRotation(EntityID id, float& x, float& y, float& z) const {
    const auto chunkIt = entityToChunk_.find(id);
    const auto indexIt = entityToIndex_.find(id);
    if (chunkIt == entityToChunk_.end() || indexIt == entityToIndex_.end() || chunkIt->second == nullptr) return false;
    const ArchetypeChunk* chunk = chunkIt->second;
    const size_t index = indexIt->second;
    if (index >= chunk->count || chunk->rotX == nullptr || chunk->rotY == nullptr || chunk->rotZ == nullptr) return false;
    x = chunk->rotX[index]; y = chunk->rotY[index]; z = chunk->rotZ[index];
    return true;
}

} // namespace NeoEngine
