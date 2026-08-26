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
} // namespace NeoEngine
