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
} // namespace NeoEngine
