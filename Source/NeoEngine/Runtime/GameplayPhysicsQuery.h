#pragma once

#include "Physics/V5/XPBDPhysicsSystem.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {
enum class GameplayPhysicsQueryError : uint8_t { None, InvalidBatch, BatchCapacity, InvalidRay, InvalidShape, InvalidMask, NoHit, Capacity, EntityMappingFailed };
struct GameplayRay2 { float originX = 0.0F; float originZ = 0.0F; float directionX = 1.0F; float directionZ = 0.0F; float maxDistance = 1.0F; CollisionMask mask = 0xFFFFFFFFU; };
struct GameplayRayHit2 { EntityID entity = 0; float distance = 0.0F; float normalX = 0.0F; float normalZ = 0.0F; };
struct GameplayOverlapCircle2 { float centerX = 0.0F; float centerZ = 0.0F; float radius = 0.0F; CollisionMask mask = 0xFFFFFFFFU; };

// Read-only gameplay query seam over XPBD's active flattened collider snapshot.
// It never calls Step, changes layers, changes constraints, or writes ECS state.
class GameplayPhysicsQuery {
public:
    static constexpr size_t kMaxOverlapHits = 256;
    static constexpr size_t kMaxOverlapBatch = 32;
    bool Raycast(const XPBDPhysicsSystem& physics, const GameplayRay2& ray, GameplayRayHit2& hit);
    bool OverlapCircle(const XPBDPhysicsSystem& physics, const GameplayOverlapCircle2& circle, std::vector<EntityID>& entities);
    bool OverlapCircleSet(const XPBDPhysicsSystem& physics, const std::vector<GameplayOverlapCircle2>& circles, std::vector<std::vector<EntityID>>& entitySets);
    [[nodiscard]] GameplayPhysicsQueryError LastError() const { return lastError_; }
private:
    GameplayPhysicsQueryError lastError_ = GameplayPhysicsQueryError::None;
};
} // namespace NeoEngine
