#include "Runtime/GameplayTriggerTracker.h"

#include <algorithm>
#include <cmath>

namespace NeoEngine {
bool GameplayTriggerTracker::Initialize(GameplayTriggerCircleConfig config) {
    if (!std::isfinite(config.centerX) || !std::isfinite(config.centerZ) || !std::isfinite(config.radius) || config.radius < 0.0F || config.mask == COLLISION_LAYER_NONE) { initialized_ = false; lastError_ = GameplayTriggerTrackerError::InvalidConfiguration; return false; }
    config_ = config; active_.clear(); delta_ = {}; initialized_ = true; lastError_ = GameplayTriggerTrackerError::None; return true;
}
bool GameplayTriggerTracker::Update(const XPBDPhysicsSystem& physics) {
    if (!initialized_) { lastError_ = GameplayTriggerTrackerError::NotInitialized; return false; }
    std::vector<EntityID> candidate; if (!query_.OverlapCircle(physics, {config_.centerX, config_.centerZ, config_.radius, config_.mask}, candidate)) { lastError_ = GameplayTriggerTrackerError::QueryFailed; return false; }
    std::sort(candidate.begin(), candidate.end()); candidate.erase(std::unique(candidate.begin(), candidate.end()), candidate.end()); GameplayTriggerDelta candidateDelta{};
    std::set_difference(candidate.begin(), candidate.end(), active_.begin(), active_.end(), std::back_inserter(candidateDelta.entered)); std::set_difference(active_.begin(), active_.end(), candidate.begin(), candidate.end(), std::back_inserter(candidateDelta.exited));
    active_ = std::move(candidate); delta_ = std::move(candidateDelta); lastError_ = GameplayTriggerTrackerError::None; return true;
}
} // namespace NeoEngine
