#pragma once

#include "Core/ECS/ArchetypeManager.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace NeoEngine {

enum class GameplayPhysicsForceError : uint8_t {
    None,
    InvalidInput,
    UnknownBody,
    StaticBody,
    InvalidBodyState,
    Overflow,
    Capacity
};

struct GameplayPlanarForce {
    EntityID entity = 0U;
    float forceX = 0.0F;
    float forceZ = 0.0F;
};

class GameplayPhysicsForceAccumulator final {
public:
    static constexpr uint16_t kMaxBodies = 4096U;

    bool ApplyForce(ArchetypeManager& entities, EntityID entity, float forceX, float forceZ);
    bool Clear(EntityID entity);
    bool Integrate(ArchetypeManager& entities, float deltaSeconds);
    void Reset();

    [[nodiscard]] GameplayPhysicsForceError LastError() const { return lastError_; }
    [[nodiscard]] size_t ActiveCount() const { return activeCount_; }

private:
    struct Entry {
        bool active = false;
        EntityID entity = 0U;
        float forceX = 0.0F;
        float forceZ = 0.0F;
    };

    static bool Finite(float value);
    Entry* Find(EntityID entity);
    const Entry* Find(EntityID entity) const;
    Entry* Allocate(EntityID entity);

    std::array<Entry, kMaxBodies> entries_{};
    uint16_t activeCount_ = 0U;
    GameplayPhysicsForceError lastError_ = GameplayPhysicsForceError::None;
};

} // namespace NeoEngine
