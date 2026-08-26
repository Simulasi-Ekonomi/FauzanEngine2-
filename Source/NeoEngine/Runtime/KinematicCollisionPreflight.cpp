#include "Runtime/KinematicCollisionPreflight.h"

#include <cmath>

namespace NeoEngine {
bool KinematicCollisionPreflight::Initialize(KinematicCollisionPreflightConfig config) {
    if (config.mask == COLLISION_LAYER_NONE || !std::isfinite(config.clearance) || config.clearance < 0.0F) { initialized_ = false; lastError_ = KinematicCollisionPreflightError::InvalidConfiguration; return false; }
    config_ = config; initialized_ = true; lastError_ = KinematicCollisionPreflightError::None; return true;
}
bool KinematicCollisionPreflight::Probe(const XPBDPhysicsSystem& physics, const SceneWorld& world, SceneEntity entity, KinematicPlanarInput input, float seconds, const KinematicMotionController& motion, KinematicCollisionPreflightProbe& output) {
    if (!initialized_) { lastError_ = KinematicCollisionPreflightError::NotInitialized; return false; }
    if (!std::isfinite(input.x) || !std::isfinite(input.z) || !std::isfinite(seconds) || seconds <= 0.0F || seconds > motion.MaxStepSeconds()) { lastError_ = KinematicCollisionPreflightError::InvalidInput; return false; }
    const Transform3* transform = world.GetLocalTransform(entity); if (transform == nullptr) { lastError_ = KinematicCollisionPreflightError::MissingTransform; return false; }
    KinematicCollisionPreflightProbe candidate{};
    const float length = std::sqrt(input.x * input.x + input.z * input.z); if (length > 0.0F) {
        const float distance = motion.UnitsPerSecond() * seconds + config_.clearance; if (!std::isfinite(distance) || distance <= 0.0F) { lastError_ = KinematicCollisionPreflightError::InvalidInput; return false; }
        GameplayRayHit2 hit{}; const bool found = query_.Raycast(physics, {transform->x, transform->z, input.x / length, input.z / length, distance, config_.mask}, hit);
        if (found) { candidate.blocked = true; candidate.blocker = hit; }
        else if (query_.LastError() != GameplayPhysicsQueryError::NoHit) { lastError_ = KinematicCollisionPreflightError::QueryFailed; return false; }
    }
    output = candidate; lastError_ = KinematicCollisionPreflightError::None; return true;
}
bool KinematicCollisionPreflight::Step(const XPBDPhysicsSystem& physics, SceneWorld& world, SceneEntity entity, KinematicPlanarInput input, float seconds, KinematicMotionController& motion) {
    KinematicCollisionPreflightProbe probe{}; if (!Probe(physics, world, entity, input, seconds, motion, probe)) return false;
    if (probe.blocked) { lastError_ = KinematicCollisionPreflightError::Blocked; return false; }
    if (!motion.Step(world, entity, input, seconds)) { lastError_ = KinematicCollisionPreflightError::MotionFailed; return false; }
    lastError_ = KinematicCollisionPreflightError::None; return true;
}
} // namespace NeoEngine
