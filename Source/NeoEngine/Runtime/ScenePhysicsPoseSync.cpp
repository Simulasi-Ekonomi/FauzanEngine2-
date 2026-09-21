#include "Runtime/ScenePhysicsPoseSync.h"

#include <cmath>
#include <new>

namespace NeoEngine {
bool ScenePhysicsPoseSync::Bind(SceneEntity sceneEntity, EntityID physicsEntity) {
    if (sceneEntity.index == 0xFFFFU) { lastError_ = ScenePhysicsPoseSyncError::InvalidSceneEntity; return false; }
    if (physicsEntity == 0xFFFFFFFFU) { lastError_ = ScenePhysicsPoseSyncError::InvalidPhysicsEntity; return false; }
    if (bindings_.size() >= kMaxBindings || bindings_.capacity() > kMaxBindings) { lastError_ = ScenePhysicsPoseSyncError::Capacity; return false; }
    for (const Binding& binding : bindings_) {
        if (binding.scene.index == 0xFFFFU || binding.physics == 0xFFFFFFFFU) { lastError_ = ScenePhysicsPoseSyncError::InvalidPhysicsEntity; return false; }
        if (binding.scene == sceneEntity) { lastError_ = ScenePhysicsPoseSyncError::DuplicateSceneEntity; return false; }
        if (binding.physics == physicsEntity) { lastError_ = ScenePhysicsPoseSyncError::DuplicatePhysicsEntity; return false; }
    }
    try {
        bindings_.push_back({sceneEntity, physicsEntity});
    } catch (...) {
        lastError_ = ScenePhysicsPoseSyncError::Capacity;
        return false;
    }
    if (bindings_.size() > kMaxBindings) {
        bindings_.pop_back();
        lastError_ = ScenePhysicsPoseSyncError::Capacity;
        return false;
    }
    lastError_ = ScenePhysicsPoseSyncError::None;
    return true;
}
bool ScenePhysicsPoseSync::Sync(const SceneWorld& world, ArchetypeManager& entities) {
    if (bindings_.size() > kMaxBindings || bindings_.capacity() > kMaxBindings) {
        lastError_ = ScenePhysicsPoseSyncError::Capacity;
        return false;
    }
    struct Candidate { EntityID physics = 0; float x = 0.0F; float z = 0.0F; }; std::vector<Candidate> candidate;
    try { candidate.reserve(bindings_.size()); } catch (...) { lastError_ = ScenePhysicsPoseSyncError::Capacity; return false; }
    for (const Binding& binding : bindings_) {
        if (binding.scene.index == 0xFFFFU || binding.physics == 0xFFFFFFFFU) { lastError_ = ScenePhysicsPoseSyncError::InvalidPhysicsEntity; return false; }
        const Transform3* transform = world.GetTransform(binding.scene);
        if (transform == nullptr) { lastError_ = ScenePhysicsPoseSyncError::MissingWorldTransform; return false; }
        if (!std::isfinite(transform->x) || !std::isfinite(transform->z)) { lastError_ = ScenePhysicsPoseSyncError::InvalidTransform; return false; }
        if (!entities.HasPosition(binding.physics)) { lastError_ = ScenePhysicsPoseSyncError::MissingPhysicsPosition; return false; }
        if (candidate.size() >= kMaxBindings) { lastError_ = ScenePhysicsPoseSyncError::Capacity; return false; }
        try { candidate.push_back({binding.physics, transform->x, transform->z}); } catch (...) { lastError_ = ScenePhysicsPoseSyncError::Capacity; return false; }
    }
    if (candidate.size() != bindings_.size()) { lastError_ = ScenePhysicsPoseSyncError::Capacity; return false; }
    for (std::size_t i = 0; i < candidate.size(); ++i) {
        const Candidate& pose = candidate[i];
        if (i >= bindings_.size() || pose.physics != bindings_[i].physics ||
            !std::isfinite(pose.x) || !std::isfinite(pose.z)) {
            lastError_ = ScenePhysicsPoseSyncError::InvalidTransform;
            return false;
        }
    }
    for (const Candidate& pose : candidate) { entities.SetPosX(pose.physics, pose.x); entities.SetPosZ(pose.physics, pose.z); }
    lastError_ = ScenePhysicsPoseSyncError::None; return true;
}
} // namespace NeoEngine
