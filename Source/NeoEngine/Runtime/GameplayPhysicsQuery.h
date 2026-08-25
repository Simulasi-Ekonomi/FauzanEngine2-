#pragma once

#include "Physics/V5/XPBDPhysicsSystem.h"

#include <cstdint>

namespace NeoEngine {
enum class GameplayPhysicsQueryError : uint8_t { None, InvalidRay, InvalidMask, NoHit, EntityMappingFailed };
struct GameplayRay2 { float originX = 0.0F; float originZ = 0.0F; float directionX = 1.0F; float directionZ = 0.0F; float maxDistance = 1.0F; CollisionMask mask = 0xFFFFFFFFU; };
struct GameplayRayHit2 { EntityID entity = 0; float distance = 0.0F; float normalX = 0.0F; float normalZ = 0.0F; };

// Read-only gameplay query seam over XPBD's active flattened collider snapshot.
// It never calls Step, changes layers, changes constraints, or writes ECS state.
class GameplayPhysicsQuery {
public:
    bool Raycast(const XPBDPhysicsSystem& physics, const GameplayRay2& ray, GameplayRayHit2& hit);
    [[nodiscard]] GameplayPhysicsQueryError LastError() const { return lastError_; }
private:
    GameplayPhysicsQueryError lastError_ = GameplayPhysicsQueryError::None;
};
} // namespace NeoEngine
