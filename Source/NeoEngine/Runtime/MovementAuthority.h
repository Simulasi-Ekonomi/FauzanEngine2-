#pragma once

#include "SceneWorld.h"

#include <array>
#include <cstdint>

namespace NeoEngine {

enum class MovementAuthority : uint8_t { KinematicRoute, SkeletalRoot };
enum class MovementAuthorityError : uint8_t { None, InvalidEntity, Conflict };

class MovementAuthorityGate {
public:
    void BeginFrame();
    [[nodiscard]] bool Acquire(SceneEntity entity, MovementAuthority authority);
    [[nodiscard]] MovementAuthorityError LastError() const { return lastError_; }

private:
    struct Claim { uint16_t generation = 0; MovementAuthority authority = MovementAuthority::KinematicRoute; bool active = false; };
    std::array<Claim, SceneWorld::kCapacity> claims_{};
    MovementAuthorityError lastError_ = MovementAuthorityError::None;
};

} // namespace NeoEngine
