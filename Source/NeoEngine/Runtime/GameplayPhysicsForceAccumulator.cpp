#include "GameplayPhysicsForceAccumulator.h"
#include "GameplayPhysicsBody.h"

#include <cmath>
#include <limits>

namespace NeoEngine {

bool GameplayPhysicsForceAccumulator::Finite(float value) {
    return std::isfinite(value);
}

GameplayPhysicsForceAccumulator::Entry* GameplayPhysicsForceAccumulator::Find(EntityID entity) {
    for (Entry& entry : entries_) if (entry.active && entry.entity == entity) return &entry;
    return nullptr;
}

const GameplayPhysicsForceAccumulator::Entry* GameplayPhysicsForceAccumulator::Find(EntityID entity) const {
    for (const Entry& entry : entries_) if (entry.active && entry.entity == entity) return &entry;
    return nullptr;
}

GameplayPhysicsForceAccumulator::Entry* GameplayPhysicsForceAccumulator::Allocate(EntityID entity) {
    for (Entry& entry : entries_) {
        if (!entry.active) {
            entry = {};
            entry.active = true;
            entry.entity = entity;
            ++activeCount_;
            return &entry;
        }
    }
    return nullptr;
}

bool GameplayPhysicsForceAccumulator::ApplyForce(ArchetypeManager& entities, EntityID entity, float forceX, float forceZ) {
    if (!Finite(forceX) || !Finite(forceZ)) {
        lastError_ = GameplayPhysicsForceError::InvalidInput;
        return false;
    }

    GameplayCircleBodySnapshot snapshot{};
    bool found = false;
    for (ArchetypeChunk* chunk : entities.GetChunks<PositionComponent, VelocityComponent, ColliderComponent>()) {
        for (size_t index = 0U; index < chunk->count; ++index) {
            if (chunk->entities[index] != entity) continue;
            const float invMass = chunk->invMass[index];
            const float radius = chunk->radius[index];
            const float velocityX = chunk->velX[index];
            const float velocityZ = chunk->velZ[index];
            if (!Finite(invMass) || !Finite(radius) || !Finite(velocityX) || !Finite(velocityZ) ||
                invMass < 0.0F || radius <= 0.0F) {
                lastError_ = GameplayPhysicsForceError::InvalidBodyState;
                return false;
            }
            if (invMass == 0.0F) {
                lastError_ = GameplayPhysicsForceError::StaticBody;
                return false;
            }
            found = true;
            break;
        }
        if (found) break;
    }
    if (!found) {
        lastError_ = GameplayPhysicsForceError::UnknownBody;
        return false;
    }

    Entry* entry = Find(entity);
    if (entry == nullptr) {
        entry = Allocate(entity);
        if (entry == nullptr) {
            lastError_ = GameplayPhysicsForceError::Capacity;
            return false;
        }
    }

    const float nextX = entry->forceX + forceX;
    const float nextZ = entry->forceZ + forceZ;
    if (!Finite(nextX) || !Finite(nextZ)) {
        if (entry->forceX == 0.0F && entry->forceZ == 0.0F) {
            entry->active = false;
            entry->entity = 0U;
            if (activeCount_ != 0U) --activeCount_;
        }
        lastError_ = GameplayPhysicsForceError::Overflow;
        return false;
    }

    entry->forceX = nextX;
    entry->forceZ = nextZ;
    lastError_ = GameplayPhysicsForceError::None;
    return true;
}

bool GameplayPhysicsForceAccumulator::Clear(EntityID entity) {
    Entry* entry = Find(entity);
    if (entry == nullptr) {
        lastError_ = GameplayPhysicsForceError::UnknownBody;
        return false;
    }
    entry->active = false;
    entry->entity = 0U;
    entry->forceX = 0.0F;
    entry->forceZ = 0.0F;
    --activeCount_;
    lastError_ = GameplayPhysicsForceError::None;
    return true;
}

bool GameplayPhysicsForceAccumulator::Integrate(ArchetypeManager& entities, float deltaSeconds) {
    if (!Finite(deltaSeconds) || deltaSeconds <= 0.0F || deltaSeconds > 1.0F) {
        lastError_ = GameplayPhysicsForceError::InvalidInput;
        return false;
    }

    struct Pending {
        Entry* entry = nullptr;
        float velocityX = 0.0F;
        float velocityZ = 0.0F;
    };
    std::array<Pending, kMaxBodies> pending{};
    size_t pendingCount = 0U;

    for (Entry& entry : entries_) {
        if (!entry.active) continue;

        ArchetypeChunk* foundChunk = nullptr;
        size_t foundIndex = 0U;
        for (ArchetypeChunk* chunk : entities.GetChunks<PositionComponent, VelocityComponent, ColliderComponent>()) {
            for (size_t index = 0U; index < chunk->count; ++index) {
                if (chunk->entities[index] == entry.entity) {
                    foundChunk = chunk;
                    foundIndex = index;
                    break;
                }
            }
            if (foundChunk != nullptr) break;
        }

        if (foundChunk == nullptr) {
            lastError_ = GameplayPhysicsForceError::UnknownBody;
            return false;
        }

        const float invMass = foundChunk->invMass[foundIndex];
        const float velocityX = foundChunk->velX[foundIndex];
        const float velocityZ = foundChunk->velZ[foundIndex];
        if (!Finite(invMass) || !Finite(velocityX) || !Finite(velocityZ) || invMass <= 0.0F ||
            !Finite(entry.forceX) || !Finite(entry.forceZ)) {
            lastError_ = invMass == 0.0F ? GameplayPhysicsForceError::StaticBody : GameplayPhysicsForceError::InvalidBodyState;
            return false;
        }

        const double nextX = static_cast<double>(velocityX) +
            static_cast<double>(entry.forceX) * static_cast<double>(invMass) * static_cast<double>(deltaSeconds);
        const double nextZ = static_cast<double>(velocityZ) +
            static_cast<double>(entry.forceZ) * static_cast<double>(invMass) * static_cast<double>(deltaSeconds);
        if (!std::isfinite(nextX) || !std::isfinite(nextZ) ||
            nextX < -static_cast<double>(std::numeric_limits<float>::max()) ||
            nextX > static_cast<double>(std::numeric_limits<float>::max()) ||
            nextZ < -static_cast<double>(std::numeric_limits<float>::max()) ||
            nextZ > static_cast<double>(std::numeric_limits<float>::max())) {
            lastError_ = GameplayPhysicsForceError::Overflow;
            return false;
        }
        pending[pendingCount++] = {&entry, static_cast<float>(nextX), static_cast<float>(nextZ)};
    }

    for (size_t i = 0U; i < pendingCount; ++i) {
        Entry& entry = *pending[i].entry;
        bool applied = false;
        for (ArchetypeChunk* chunk : entities.GetChunks<PositionComponent, VelocityComponent, ColliderComponent>()) {
            for (size_t index = 0U; index < chunk->count; ++index) {
                if (chunk->entities[index] != entry.entity) continue;
                chunk->velX[index] = pending[i].velocityX;
                chunk->velZ[index] = pending[i].velocityZ;
                applied = true;
                break;
            }
            if (applied) break;
        }
        if (!applied) {
            lastError_ = GameplayPhysicsForceError::UnknownBody;
            return false;
        }
        entry.forceX = 0.0F;
        entry.forceZ = 0.0F;
    }

    if (pendingCount != 0U) entities.MarkPhysicsDirty();
    lastError_ = GameplayPhysicsForceError::None;
    return true;
}

void GameplayPhysicsForceAccumulator::Reset() {
    entries_ = {};
    activeCount_ = 0U;
    lastError_ = GameplayPhysicsForceError::None;
}

} // namespace NeoEngine
