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
bool GameplayPhysicsQuery::OverlapCircle(const XPBDPhysicsSystem& physics, const GameplayOverlapCircle2& circle, std::vector<EntityID>& entities) {
    if (!std::isfinite(circle.centerX) || !std::isfinite(circle.centerZ) || !std::isfinite(circle.radius) || circle.radius < 0.0F) { lastError_ = GameplayPhysicsQueryError::InvalidShape; return false; }
    if (circle.mask == COLLISION_LAYER_NONE) { lastError_ = GameplayPhysicsQueryError::InvalidMask; return false; }
    const std::vector<uint32_t> raw = physics.OverlapSphere(circle.centerX, circle.centerZ, circle.radius, circle.mask);
    if (raw.size() > kMaxOverlapHits) { lastError_ = GameplayPhysicsQueryError::Capacity; return false; }
    std::vector<EntityID> candidate; candidate.reserve(raw.size()); for (const uint32_t index : raw) { EntityID entity = 0; if (!physics.TryGetEntityId(index, entity)) { lastError_ = GameplayPhysicsQueryError::EntityMappingFailed; return false; } candidate.push_back(entity); }
    entities = std::move(candidate); lastError_ = GameplayPhysicsQueryError::None; return true;
}
} // namespace NeoEngine
