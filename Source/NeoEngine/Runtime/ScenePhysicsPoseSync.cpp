#include "Runtime/ScenePhysicsPoseSync.h"

#include <cmath>
#include <new>

namespace NeoEngine {
bool ScenePhysicsPoseSync::Bind(SceneEntity sceneEntity, EntityID physicsEntity) {
    return Bind(sceneEntity, physicsEntity, false);
}

bool ScenePhysicsPoseSync::Bind(SceneEntity sceneEntity, EntityID physicsEntity, bool physicsAuthoritative) {
    if (sceneEntity.index == 0xFFFFU || physicsEntity == 0U) {
        lastError_ = ScenePhysicsPoseSyncError::InvalidSceneEntity;
        return false;
    }
    if (bindings_.size() >= kMaxBindings) {
        lastError_ = ScenePhysicsPoseSyncError::Capacity;
        return false;
    }
    for (const Binding& binding : bindings_) {
        if (binding.scene == sceneEntity) {
            lastError_ = ScenePhysicsPoseSyncError::DuplicateSceneEntity;
            return false;
        }
        if (binding.physics == physicsEntity) {
            lastError_ = ScenePhysicsPoseSyncError::DuplicatePhysicsEntity;
            return false;
        }
    }
    try {
        bindings_.push_back({sceneEntity, physicsEntity, physicsAuthoritative});
    } catch (const std::bad_alloc&) {
        lastError_ = ScenePhysicsPoseSyncError::Capacity;
        return false;
    }
    lastError_ = ScenePhysicsPoseSyncError::None;
    return true;
}

bool ScenePhysicsPoseSync::Unbind(SceneEntity sceneEntity) {
    for (auto it = bindings_.begin(); it != bindings_.end(); ++it) {
        if (it->scene == sceneEntity) {
            bindings_.erase(it);
            lastError_ = ScenePhysicsPoseSyncError::None;
            return true;
        }
    }
    lastError_ = ScenePhysicsPoseSyncError::UnknownBinding;
    return false;
}

bool ScenePhysicsPoseSync::Sync(const SceneWorld& world, ArchetypeManager& entities) {
    struct Candidate { EntityID physics = 0; float x = 0.0F; float z = 0.0F; };
    std::vector<Candidate> candidates;
    try { candidates.reserve(bindings_.size()); } catch (...) { lastError_ = ScenePhysicsPoseSyncError::Capacity; return false; }

    for (const Binding& binding : bindings_) {
        if (binding.physicsAuthoritative) continue;
        const Transform3* transform = world.GetTransform(binding.scene);
        if (transform == nullptr) {
            lastError_ = ScenePhysicsPoseSyncError::MissingWorldTransform;
            return false;
        }
        if (!std::isfinite(transform->x) || !std::isfinite(transform->z)) {
            lastError_ = ScenePhysicsPoseSyncError::InvalidTransform;
            return false;
        }
        if (!entities.HasPosition(binding.physics)) {
            lastError_ = ScenePhysicsPoseSyncError::MissingPhysicsPosition;
            return false;
        }
        try { candidates.push_back({binding.physics, transform->x, transform->z}); } catch (...) { lastError_ = ScenePhysicsPoseSyncError::Capacity; return false; }
    }

    for (const Candidate& pose : candidates) {
        entities.SetPosX(pose.physics, pose.x);
        entities.SetPosZ(pose.physics, pose.z);
    }
    lastError_ = ScenePhysicsPoseSyncError::None;
    return true;
}

bool ScenePhysicsPoseSync::SyncFromPhysics(SceneWorld& world, ArchetypeManager& entities) {
    struct Candidate { SceneEntity scene{}; Transform3 transform{}; };
    std::vector<Candidate> candidates;
    try { candidates.reserve(bindings_.size()); } catch (...) { lastError_ = ScenePhysicsPoseSyncError::Capacity; return false; }

    for (const Binding& binding : bindings_) {
        if (!binding.physicsAuthoritative) continue;
        bool found = false;
        for (ArchetypeChunk* chunk : entities.GetChunks<PositionComponent, VelocityComponent, ColliderComponent>()) {
            for (size_t index = 0U; index < chunk->count; ++index) {
                if (chunk->entities[index] != binding.physics) continue;
                const float x = chunk->posX[index];
                const float z = chunk->posZ[index];
                if (index >= chunk->count || chunk->posX == nullptr || chunk->posZ == nullptr || chunk->radius == nullptr || chunk->invMass == nullptr || chunk->entities == nullptr) { lastError_ = ScenePhysicsPoseSyncError::InvalidPhysicsPose; return false; }
                const float radius = chunk->radius[index];
                const float inverseMass = chunk->invMass[index];
                if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(radius) ||
                    !std::isfinite(inverseMass) || radius <= 0.0F || inverseMass <= 0.0F) {
                    lastError_ = ScenePhysicsPoseSyncError::InvalidPhysicsPose;
                    return false;
                }
                const Transform3* current = world.GetTransform(binding.scene);
                if (current == nullptr) {
                    lastError_ = ScenePhysicsPoseSyncError::MissingWorldTransform;
                    return false;
                }
                Transform3 next = *current;
                next.x = x;
                next.z = z;
                if (!std::isfinite(next.x) || !std::isfinite(next.z)) {
                    lastError_ = ScenePhysicsPoseSyncError::InvalidPhysicsPose;
                    return false;
                }
                try { candidates.push_back({binding.scene, next}); } catch (...) { lastError_ = ScenePhysicsPoseSyncError::Capacity; return false; }
                found = true;
                break;
            }
            if (found) break;
        }
        if (!found) {
            lastError_ = ScenePhysicsPoseSyncError::MissingPhysicsPosition;
            return false;
        }
    }

    for (const Candidate& pose : candidates) {
        if (!world.SetTransform(pose.scene, pose.transform)) {
            lastError_ = ScenePhysicsPoseSyncError::InvalidTransform;
            return false;
        }
    }

    lastError_ = ScenePhysicsPoseSyncError::None;
    return true;
}

bool ScenePhysicsPoseSync::GetPhysicsEntity(SceneEntity sceneEntity, EntityID& physicsEntity) const {
    for (const Binding& binding : bindings_) {
        if (binding.scene == sceneEntity) {
            physicsEntity = binding.physics;
            return true;
        }
    }
    return false;
}

bool ScenePhysicsPoseSync::IsPhysicsAuthoritative(SceneEntity sceneEntity) const {
    for (const Binding& binding : bindings_) {
        if (binding.scene == sceneEntity) return binding.physicsAuthoritative;
    }
    return false;
}
} // namespace NeoEngine
