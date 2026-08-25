#include "Runtime/ScenePhysicsPoseSync.h"

#include <cmath>

namespace NeoEngine {
bool ScenePhysicsPoseSync::Bind(SceneEntity sceneEntity, EntityID physicsEntity) {
    if (sceneEntity.index == 0xFFFFU) { lastError_ = ScenePhysicsPoseSyncError::InvalidSceneEntity; return false; }
    if (bindings_.size() >= kMaxBindings) { lastError_ = ScenePhysicsPoseSyncError::Capacity; return false; }
    for (const Binding& binding : bindings_) { if (binding.scene == sceneEntity) { lastError_ = ScenePhysicsPoseSyncError::DuplicateSceneEntity; return false; } if (binding.physics == physicsEntity) { lastError_ = ScenePhysicsPoseSyncError::DuplicatePhysicsEntity; return false; } }
    bindings_.push_back({sceneEntity, physicsEntity}); lastError_ = ScenePhysicsPoseSyncError::None; return true;
}
bool ScenePhysicsPoseSync::Sync(const SceneWorld& world, ArchetypeManager& entities) {
    struct Candidate { EntityID physics = 0; float x = 0.0F; float z = 0.0F; }; std::vector<Candidate> candidate; candidate.reserve(bindings_.size());
    for (const Binding& binding : bindings_) { const Transform3* transform = world.GetTransform(binding.scene); if (transform == nullptr) { lastError_ = ScenePhysicsPoseSyncError::MissingWorldTransform; return false; } if (!std::isfinite(transform->x) || !std::isfinite(transform->z)) { lastError_ = ScenePhysicsPoseSyncError::InvalidTransform; return false; } if (!entities.HasPosition(binding.physics)) { lastError_ = ScenePhysicsPoseSyncError::MissingPhysicsPosition; return false; } candidate.push_back({binding.physics, transform->x, transform->z}); }
    for (const Candidate& pose : candidate) { entities.SetPosX(pose.physics, pose.x); entities.SetPosZ(pose.physics, pose.z); }
    lastError_ = ScenePhysicsPoseSyncError::None; return true;
}
} // namespace NeoEngine
