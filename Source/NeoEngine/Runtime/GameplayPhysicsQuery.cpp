#include "Runtime/GameplayPhysicsQuery.h"

#include <cmath>

namespace NeoEngine {
bool GameplayPhysicsQuery::Raycast(const XPBDPhysicsSystem& physics, const GameplayRay2& ray, GameplayRayHit2& hit) {
    if (!std::isfinite(ray.originX) || !std::isfinite(ray.originZ) || !std::isfinite(ray.directionX) || !std::isfinite(ray.directionZ) || !std::isfinite(ray.maxDistance) || ray.maxDistance < 0.0F || ((ray.directionX * ray.directionX) + (ray.directionZ * ray.directionZ)) <= 1.0e-12F) { lastError_ = GameplayPhysicsQueryError::InvalidRay; return false; }
    if (ray.mask == COLLISION_LAYER_NONE) { lastError_ = GameplayPhysicsQueryError::InvalidMask; return false; }
    RayHit raw{}; if (!physics.Raycast(ray.originX, ray.originZ, ray.directionX, ray.directionZ, ray.maxDistance, raw, ray.mask)) { lastError_ = GameplayPhysicsQueryError::NoHit; return false; }
    GameplayRayHit2 candidate{}; if (!physics.TryGetEntityId(raw.entityIdx, candidate.entity)) { lastError_ = GameplayPhysicsQueryError::EntityMappingFailed; return false; }
    candidate.distance = raw.distance; candidate.normalX = raw.normalX; candidate.normalZ = raw.normalZ; hit = candidate; lastError_ = GameplayPhysicsQueryError::None; return true;
}
} // namespace NeoEngine
