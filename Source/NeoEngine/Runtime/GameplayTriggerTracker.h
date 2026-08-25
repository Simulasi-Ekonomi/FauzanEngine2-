#pragma once

#include "GameplayPhysicsQuery.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {
enum class GameplayTriggerTrackerError : uint8_t { None, InvalidConfiguration, NotInitialized, QueryFailed };
struct GameplayTriggerCircleConfig { float centerX = 0.0F; float centerZ = 0.0F; float radius = 0.5F; CollisionMask mask = 0xFFFFFFFFU; };
struct GameplayTriggerDelta { std::vector<EntityID> entered; std::vector<EntityID> exited; };

// Read-only trigger state: it observes overlap deltas but owns no callbacks, solver, ECS,
// entity flags, collision layers, or transform writes.
class GameplayTriggerTracker {
public:
    bool Initialize(GameplayTriggerCircleConfig config);
    bool Update(const XPBDPhysicsSystem& physics);
    [[nodiscard]] const std::vector<EntityID>& ActiveEntities() const { return active_; }
    [[nodiscard]] const GameplayTriggerDelta& LastDelta() const { return delta_; }
    [[nodiscard]] GameplayTriggerTrackerError LastError() const { return lastError_; }
private:
    GameplayTriggerCircleConfig config_{};
    GameplayPhysicsQuery query_{};
    std::vector<EntityID> active_;
    GameplayTriggerDelta delta_{};
    GameplayTriggerTrackerError lastError_ = GameplayTriggerTrackerError::InvalidConfiguration;
    bool initialized_ = false;
};
} // namespace NeoEngine
