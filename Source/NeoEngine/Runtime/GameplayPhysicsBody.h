#pragma once

#include "Core/ECS/ArchetypeManager.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {
enum class GameplayPhysicsBodyType : uint8_t { Static, Dynamic };
enum class GameplayPhysicsBodyError : uint8_t { None, InvalidConfiguration, Capacity, DuplicateBody, UnknownBody, InvalidBodyState, StaticBody, VelocityOverflow };
struct GameplayCircleBodyConfig { GameplayPhysicsBodyType type = GameplayPhysicsBodyType::Static; float positionX = 0.0F; float positionZ = 0.0F; float velocityX = 0.0F; float velocityZ = 0.0F; float radius = 0.5F; float mass = 1.0F; };
struct GameplayCircleBodySnapshot { GameplayPhysicsBodyType type = GameplayPhysicsBodyType::Static; float positionX = 0.0F; float positionZ = 0.0F; float velocityX = 0.0F; float velocityZ = 0.0F; float radius = 0.0F; float inverseMass = 0.0F; };
struct GameplayPlanarVelocityCommand { EntityID entity = 0; float velocityX = 0.0F; float velocityZ = 0.0F; };
struct GameplayPlanarImpulseCommand { EntityID entity = 0; float impulseX = 0.0F; float impulseZ = 0.0F; };

// Creates only canonical ECS Position/Velocity/Collider components. The caller retains
// XPBD Step timing, query timing, collision layers, movement authority, and destruction.
class GameplayPhysicsBodyBuilder {
public:
    static constexpr size_t kMaxVelocityCommands = 64U;
    bool CreateCircleBody(ArchetypeManager& entities, const GameplayCircleBodyConfig& config, EntityID& entity);
    bool SnapshotCircleBody(ArchetypeManager& entities, EntityID entity, GameplayCircleBodySnapshot& snapshot);
    bool SetDynamicPlanarVelocity(ArchetypeManager& entities, EntityID entity, float velocityX, float velocityZ);
    bool SetDynamicPlanarVelocitySet(ArchetypeManager& entities, const std::vector<GameplayPlanarVelocityCommand>& commands);
    bool ApplyDynamicPlanarImpulse(ArchetypeManager& entities, EntityID entity, float impulseX, float impulseZ);
    bool ApplyDynamicPlanarImpulseSet(ArchetypeManager& entities, const std::vector<GameplayPlanarImpulseCommand>& commands);
    [[nodiscard]] GameplayPhysicsBodyError LastError() const { return lastError_; }
private:
    GameplayPhysicsBodyError lastError_ = GameplayPhysicsBodyError::None;
};
} // namespace NeoEngine
