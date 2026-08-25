#pragma once

#include "Core/ECS/ArchetypeManager.h"

#include <cstdint>

namespace NeoEngine {
enum class GameplayPhysicsBodyType : uint8_t { Static, Dynamic };
enum class GameplayPhysicsBodyError : uint8_t { None, InvalidConfiguration };
struct GameplayCircleBodyConfig { GameplayPhysicsBodyType type = GameplayPhysicsBodyType::Static; float positionX = 0.0F; float positionZ = 0.0F; float velocityX = 0.0F; float velocityZ = 0.0F; float radius = 0.5F; float mass = 1.0F; };

// Creates only canonical ECS Position/Velocity/Collider components. The caller retains
// XPBD Step timing, query timing, collision layers, movement authority, and destruction.
class GameplayPhysicsBodyBuilder {
public:
    bool CreateCircleBody(ArchetypeManager& entities, const GameplayCircleBodyConfig& config, EntityID& entity);
    [[nodiscard]] GameplayPhysicsBodyError LastError() const { return lastError_; }
private:
    GameplayPhysicsBodyError lastError_ = GameplayPhysicsBodyError::None;
};
} // namespace NeoEngine
