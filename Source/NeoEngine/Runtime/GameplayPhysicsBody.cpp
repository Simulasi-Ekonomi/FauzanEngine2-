#include "Runtime/GameplayPhysicsBody.h"

#include <cmath>

namespace NeoEngine {
bool GameplayPhysicsBodyBuilder::CreateCircleBody(ArchetypeManager& entities, const GameplayCircleBodyConfig& config, EntityID& entity) {
    if (!std::isfinite(config.positionX) || !std::isfinite(config.positionZ) || !std::isfinite(config.velocityX) || !std::isfinite(config.velocityZ) || !std::isfinite(config.radius) || config.radius <= 0.0F || (config.type == GameplayPhysicsBodyType::Dynamic && (!std::isfinite(config.mass) || config.mass <= 0.0F)) || (config.type == GameplayPhysicsBodyType::Static && (config.velocityX != 0.0F || config.velocityZ != 0.0F))) { lastError_ = GameplayPhysicsBodyError::InvalidConfiguration; return false; }
    const float inverseMass = config.type == GameplayPhysicsBodyType::Static ? 0.0F : 1.0F / config.mass;
    if (!std::isfinite(inverseMass)) { lastError_ = GameplayPhysicsBodyError::InvalidConfiguration; return false; }
    const EntityID candidate = entities.CreateEntity(COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER);
    entities.SetPosX(candidate, config.positionX); entities.SetPosZ(candidate, config.positionZ); entities.SetVelX(candidate, config.type == GameplayPhysicsBodyType::Static ? 0.0F : config.velocityX); entities.SetVelZ(candidate, config.type == GameplayPhysicsBodyType::Static ? 0.0F : config.velocityZ); entities.SetRadius(candidate, config.radius); entities.SetInvMass(candidate, inverseMass);
    entity = candidate; lastError_ = GameplayPhysicsBodyError::None; return true;
}
bool GameplayPhysicsBodyBuilder::SnapshotCircleBody(ArchetypeManager& entities, EntityID entity, GameplayCircleBodySnapshot& snapshot) {
    for (ArchetypeChunk* chunk : entities.GetChunks<PositionComponent, VelocityComponent, ColliderComponent>()) {
        for (size_t index = 0; index < chunk->count; ++index) {
            if (chunk->entities[index] != entity) continue;
            const float positionX = chunk->posX[index], positionZ = chunk->posZ[index], velocityX = chunk->velX[index], velocityZ = chunk->velZ[index], radius = chunk->radius[index], inverseMass = chunk->invMass[index];
            if (!std::isfinite(positionX) || !std::isfinite(positionZ) || !std::isfinite(velocityX) || !std::isfinite(velocityZ) || !std::isfinite(radius) || !std::isfinite(inverseMass) || radius <= 0.0F || inverseMass < 0.0F) { lastError_ = GameplayPhysicsBodyError::InvalidBodyState; return false; }
            if ((inverseMass == 0.0F && (velocityX != 0.0F || velocityZ != 0.0F)) || (inverseMass > 0.0F && !std::isfinite(1.0F / inverseMass))) { lastError_ = GameplayPhysicsBodyError::InvalidBodyState; return false; }
            snapshot = {inverseMass == 0.0F ? GameplayPhysicsBodyType::Static : GameplayPhysicsBodyType::Dynamic, positionX, positionZ, velocityX, velocityZ, radius, inverseMass}; lastError_ = GameplayPhysicsBodyError::None; return true;
        }
    }
    lastError_ = GameplayPhysicsBodyError::UnknownBody; return false;
}
bool GameplayPhysicsBodyBuilder::SetDynamicPlanarVelocity(ArchetypeManager& entities, EntityID entity, float velocityX, float velocityZ) {
    if (!std::isfinite(velocityX) || !std::isfinite(velocityZ)) { lastError_ = GameplayPhysicsBodyError::InvalidConfiguration; return false; }
    for (ArchetypeChunk* chunk : entities.GetChunks<PositionComponent, VelocityComponent, ColliderComponent>()) {
        for (size_t index = 0; index < chunk->count; ++index) {
            if (chunk->entities[index] != entity) continue;
            const float inverseMass = chunk->invMass[index], radius = chunk->radius[index];
            if (!std::isfinite(inverseMass) || !std::isfinite(radius) || inverseMass < 0.0F || radius <= 0.0F) { lastError_ = GameplayPhysicsBodyError::InvalidBodyState; return false; }
            if (inverseMass == 0.0F) { lastError_ = GameplayPhysicsBodyError::StaticBody; return false; }
            chunk->velX[index] = velocityX; chunk->velZ[index] = velocityZ; entities.MarkPhysicsDirty(); lastError_ = GameplayPhysicsBodyError::None; return true;
        }
    }
    lastError_ = GameplayPhysicsBodyError::UnknownBody; return false;
}
bool GameplayPhysicsBodyBuilder::SetDynamicPlanarVelocitySet(ArchetypeManager& entities, const std::vector<GameplayPlanarVelocityCommand>& commands) {
    if (commands.empty()) { lastError_ = GameplayPhysicsBodyError::InvalidConfiguration; return false; }
    if (commands.size() > kMaxVelocityCommands) { lastError_ = GameplayPhysicsBodyError::Capacity; return false; }
    struct Pending { ArchetypeChunk* chunk = nullptr; size_t index = 0U; float velocityX = 0.0F; float velocityZ = 0.0F; };
    std::vector<Pending> pending; pending.reserve(commands.size());
    for (size_t commandIndex = 0U; commandIndex < commands.size(); ++commandIndex) {
        const GameplayPlanarVelocityCommand& command = commands[commandIndex];
        if (!std::isfinite(command.velocityX) || !std::isfinite(command.velocityZ)) { lastError_ = GameplayPhysicsBodyError::InvalidConfiguration; return false; }
        for (size_t prior = 0U; prior < commandIndex; ++prior) if (commands[prior].entity == command.entity) { lastError_ = GameplayPhysicsBodyError::DuplicateBody; return false; }
        ArchetypeChunk* foundChunk = nullptr; size_t foundIndex = 0U;
        for (ArchetypeChunk* chunk : entities.GetChunks<PositionComponent, VelocityComponent, ColliderComponent>()) {
            for (size_t index = 0U; index < chunk->count; ++index) if (chunk->entities[index] == command.entity) { foundChunk = chunk; foundIndex = index; break; }
            if (foundChunk != nullptr) break;
        }
        if (foundChunk == nullptr) { lastError_ = GameplayPhysicsBodyError::UnknownBody; return false; }
        const float inverseMass = foundChunk->invMass[foundIndex], radius = foundChunk->radius[foundIndex];
        if (!std::isfinite(inverseMass) || !std::isfinite(radius) || inverseMass < 0.0F || radius <= 0.0F) { lastError_ = GameplayPhysicsBodyError::InvalidBodyState; return false; }
        if (inverseMass == 0.0F) { lastError_ = GameplayPhysicsBodyError::StaticBody; return false; }
        pending.push_back({foundChunk, foundIndex, command.velocityX, command.velocityZ});
    }
    for (const Pending& command : pending) { command.chunk->velX[command.index] = command.velocityX; command.chunk->velZ[command.index] = command.velocityZ; }
    entities.MarkPhysicsDirty(); lastError_ = GameplayPhysicsBodyError::None; return true;
}

bool GameplayPhysicsBodyBuilder::ApplyDynamicPlanarImpulse(ArchetypeManager& entities, EntityID entity, float impulseX, float impulseZ) {
    if (!std::isfinite(impulseX) || !std::isfinite(impulseZ)) { lastError_ = GameplayPhysicsBodyError::InvalidConfiguration; return false; }
    for (ArchetypeChunk* chunk : entities.GetChunks<PositionComponent, VelocityComponent, ColliderComponent>()) {
        for (size_t index = 0U; index < chunk->count; ++index) {
            if (chunk->entities[index] != entity) continue;
            const float inverseMass = chunk->invMass[index], radius = chunk->radius[index], velocityX = chunk->velX[index], velocityZ = chunk->velZ[index];
            if (!std::isfinite(inverseMass) || !std::isfinite(radius) || !std::isfinite(velocityX) || !std::isfinite(velocityZ) || inverseMass < 0.0F || radius <= 0.0F) { lastError_ = GameplayPhysicsBodyError::InvalidBodyState; return false; }
            if (inverseMass == 0.0F) { lastError_ = GameplayPhysicsBodyError::StaticBody; return false; }
            const float nextVelocityX = velocityX + impulseX * inverseMass, nextVelocityZ = velocityZ + impulseZ * inverseMass;
            if (!std::isfinite(nextVelocityX) || !std::isfinite(nextVelocityZ)) { lastError_ = GameplayPhysicsBodyError::VelocityOverflow; return false; }
            chunk->velX[index] = nextVelocityX; chunk->velZ[index] = nextVelocityZ; entities.MarkPhysicsDirty(); lastError_ = GameplayPhysicsBodyError::None; return true;
        }
    }
    lastError_ = GameplayPhysicsBodyError::UnknownBody; return false;
}

bool GameplayPhysicsBodyBuilder::ApplyDynamicPlanarImpulseSet(ArchetypeManager& entities, const std::vector<GameplayPlanarImpulseCommand>& commands) {
    if (commands.empty()) { lastError_ = GameplayPhysicsBodyError::InvalidConfiguration; return false; }
    if (commands.size() > kMaxVelocityCommands) { lastError_ = GameplayPhysicsBodyError::Capacity; return false; }
    struct Pending { ArchetypeChunk* chunk = nullptr; size_t index = 0U; float velocityX = 0.0F; float velocityZ = 0.0F; };
    std::vector<Pending> pending; pending.reserve(commands.size());
    for (size_t commandIndex = 0U; commandIndex < commands.size(); ++commandIndex) {
        const GameplayPlanarImpulseCommand& command = commands[commandIndex];
        if (!std::isfinite(command.impulseX) || !std::isfinite(command.impulseZ)) { lastError_ = GameplayPhysicsBodyError::InvalidConfiguration; return false; }
        for (size_t prior = 0U; prior < commandIndex; ++prior) if (commands[prior].entity == command.entity) { lastError_ = GameplayPhysicsBodyError::DuplicateBody; return false; }
        ArchetypeChunk* foundChunk = nullptr; size_t foundIndex = 0U;
        for (ArchetypeChunk* chunk : entities.GetChunks<PositionComponent, VelocityComponent, ColliderComponent>()) {
            for (size_t index = 0U; index < chunk->count; ++index) if (chunk->entities[index] == command.entity) { foundChunk = chunk; foundIndex = index; break; }
            if (foundChunk != nullptr) break;
        }
        if (foundChunk == nullptr) { lastError_ = GameplayPhysicsBodyError::UnknownBody; return false; }
        const float inverseMass = foundChunk->invMass[foundIndex], radius = foundChunk->radius[foundIndex], velocityX = foundChunk->velX[foundIndex], velocityZ = foundChunk->velZ[foundIndex];
        if (!std::isfinite(inverseMass) || !std::isfinite(radius) || !std::isfinite(velocityX) || !std::isfinite(velocityZ) || inverseMass < 0.0F || radius <= 0.0F) { lastError_ = GameplayPhysicsBodyError::InvalidBodyState; return false; }
        if (inverseMass == 0.0F) { lastError_ = GameplayPhysicsBodyError::StaticBody; return false; }
        const float nextVelocityX = velocityX + command.impulseX * inverseMass, nextVelocityZ = velocityZ + command.impulseZ * inverseMass;
        if (!std::isfinite(nextVelocityX) || !std::isfinite(nextVelocityZ)) { lastError_ = GameplayPhysicsBodyError::VelocityOverflow; return false; }
        pending.push_back({foundChunk, foundIndex, nextVelocityX, nextVelocityZ});
    }
    for (const Pending& command : pending) { command.chunk->velX[command.index] = command.velocityX; command.chunk->velZ[command.index] = command.velocityZ; }
    entities.MarkPhysicsDirty(); lastError_ = GameplayPhysicsBodyError::None; return true;
}
} // namespace NeoEngine
