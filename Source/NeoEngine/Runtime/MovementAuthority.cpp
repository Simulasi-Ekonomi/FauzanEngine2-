#include "MovementAuthority.h"

namespace NeoEngine {

void MovementAuthorityGate::BeginFrame() {
    for (Claim& claim : claims_) claim = {};
    lastError_ = MovementAuthorityError::None;
}

bool MovementAuthorityGate::Acquire(const SceneEntity entity, const MovementAuthority authority) {
    if (entity.index >= SceneWorld::kCapacity || entity.index == 0xFFFFU) { lastError_ = MovementAuthorityError::InvalidEntity; return false; }
    Claim& claim = claims_[entity.index];
    if (!claim.active || claim.generation != entity.generation) { claim = {entity.generation, authority, true}; lastError_ = MovementAuthorityError::None; return true; }
    if (claim.authority == authority) { lastError_ = MovementAuthorityError::None; return true; }
    lastError_ = MovementAuthorityError::Conflict;
    return false;
}

} // namespace NeoEngine
