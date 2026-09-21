#include "CanonicalRuntimeWorld.h"

#include <cmath>
#include <limits>

namespace NeoEngine {

CanonicalRuntimeWorld::CanonicalRuntimeWorld() : resources_(assets_) {}

bool CanonicalRuntimeWorld::IsPhysicsBody(uint32_t componentMask) {
    constexpr uint32_t required = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;
    return (componentMask & required) == required;
}

bool CanonicalRuntimeWorld::ValidateTransform(const Transform3& transform) const {
    return std::isfinite(transform.x) && std::isfinite(transform.y) && std::isfinite(transform.z) &&
           std::isfinite(transform.rx) && std::isfinite(transform.ry) && std::isfinite(transform.rz) &&
           std::isfinite(transform.sx) && std::isfinite(transform.sy) && std::isfinite(transform.sz) &&
           transform.sx > 0.0F && transform.sy > 0.0F && transform.sz > 0.0F;
}

bool CanonicalRuntimeWorld::ValidateEntity(const CanonicalEntity& entity) const {
    if (!entity.active || entity.scene.index == 0xFFFFU) return false;
    if (scene_.GetTransform(entity.scene) == nullptr) return false;
    return !entity.hasECS || ecs_.HasEntity(entity.ecs);
}

bool CanonicalRuntimeWorld::CreateEntity(const Transform3& transform, uint32_t componentMask,
                                         CanonicalTransformAuthority authority,
                                         CanonicalEntity& outEntity) {
    if (!ValidateTransform(transform)) { lastError_ = CanonicalWorldError::InvalidTransform; return false; }
    if (bindingCount_ >= kMaxBindings) { lastError_ = CanonicalWorldError::Capacity; return false; }

    SceneEntity sceneEntity{};
    if (!scene_.Create(sceneEntity) || !scene_.SetTransform(sceneEntity, transform)) {
        lastError_ = CanonicalWorldError::InvalidEntity;
        return false;
    }

    CanonicalEntity candidate{sceneEntity, 0xFFFFFFFFU, componentMask, authority, false, true};

    if (componentMask != 0U) {
        const EntityID physicsEntity = ecs_.CreateEntity(componentMask);
        if (physicsEntity == std::numeric_limits<EntityID>::max()) {
            scene_.Destroy(sceneEntity);
            lastError_ = CanonicalWorldError::PhysicsCreationFailed;
            return false;
        }
        candidate.ecs = physicsEntity;
        candidate.hasECS = true;
        if ((componentMask & COMP_POSITION) != 0U) {
            ecs_.SetPosX(physicsEntity, transform.x);
            ecs_.SetPosZ(physicsEntity, transform.z);
        }
    }

    bindings_[bindingCount_++].entity = candidate;
    outEntity = candidate;
    lastError_ = CanonicalWorldError::None;
    return true;
}

bool CanonicalRuntimeWorld::GetEntity(SceneEntity sceneEntity, CanonicalEntity& outEntity) const {
    if (sceneEntity.index == 0xFFFFU) return false;
    for (uint16_t i = 0U; i < bindingCount_; ++i) {
        const CanonicalEntity& candidate = bindings_[i].entity;
        if (candidate.active && candidate.scene == sceneEntity) {
            outEntity = candidate;
            return true;
        }
    }
    return false;
}

bool CanonicalRuntimeWorld::DestroyEntity(CanonicalEntity entity) {
    if (!ValidateEntity(entity)) { lastError_ = CanonicalWorldError::InvalidEntity; return false; }
    // Destroy the Scene entity first. ECS destruction has no failure channel, so doing
    // it first could leave the canonical world split if SceneWorld rejects the handle.
    if (!scene_.Destroy(entity.scene)) { lastError_ = CanonicalWorldError::InvalidEntity; return false; }
    if (entity.hasECS) ecs_.DestroyEntity(entity.ecs);

    for (uint16_t i = 0U; i < bindingCount_; ++i) {
        if (bindings_[i].entity.scene == entity.scene) {
            bindings_[i] = bindings_[bindingCount_ - 1U];
            bindings_[bindingCount_ - 1U] = {};
            --bindingCount_;
            break;
        }
    }
    lastError_ = CanonicalWorldError::None;
    return true;
}

bool CanonicalRuntimeWorld::SetTransform(CanonicalEntity entity, const Transform3& transform) {
    if (!ValidateEntity(entity)) { lastError_ = CanonicalWorldError::InvalidEntity; return false; }
    if (!ValidateTransform(transform)) { lastError_ = CanonicalWorldError::InvalidTransform; return false; }
    if (entity.authority != CanonicalTransformAuthority::Scene) {
        lastError_ = CanonicalWorldError::TransformAuthorityViolation;
        return false;
    }
    if (!scene_.SetTransform(entity.scene, transform)) { lastError_ = CanonicalWorldError::InvalidTransform; return false; }

    if (entity.hasECS && (entity.componentMask & COMP_POSITION) != 0U) {
        ecs_.SetPosX(entity.ecs, transform.x);
        ecs_.SetPosZ(entity.ecs, transform.z);
    }
    lastError_ = CanonicalWorldError::None;
    return true;
}

bool CanonicalRuntimeWorld::BindMesh(const SceneMeshInstance& instance) {
    if (!scene_.GetTransform(instance.entity)) { lastError_ = CanonicalWorldError::InvalidEntity; return false; }
    if (!meshes_.Add(instance)) { lastError_ = CanonicalWorldError::MeshBindingFailed; return false; }
    lastError_ = CanonicalWorldError::None;
    return true;
}

bool CanonicalRuntimeWorld::SyncSceneToPhysics() {
    for (uint16_t i = 0U; i < bindingCount_; ++i) {
        const CanonicalEntity& entity = bindings_[i].entity;
        if (!entity.active || !entity.hasECS || !IsPhysicsBody(entity.componentMask) ||
            entity.authority != CanonicalTransformAuthority::Scene) continue;
        const Transform3* transform = scene_.GetTransform(entity.scene);
        if (!transform || (entity.componentMask & COMP_POSITION) == 0U) {
            lastError_ = CanonicalWorldError::PhysicsSyncFailed;
            return false;
        }
        ecs_.SetPosX(entity.ecs, transform->x);
        ecs_.SetPosZ(entity.ecs, transform->z);
    }
    return true;
}

bool CanonicalRuntimeWorld::ReadBackPhysicsToScene() {
    const auto chunks = ecs_.GetChunks<PositionComponent>();
    for (uint16_t bindingIndex = 0U; bindingIndex < bindingCount_; ++bindingIndex) {
        const CanonicalEntity& entity = bindings_[bindingIndex].entity;
        if (!entity.active || !entity.hasECS || !IsPhysicsBody(entity.componentMask) ||
            entity.authority != CanonicalTransformAuthority::Physics) continue;

        bool found = false;
        for (const ArchetypeChunk* chunk : chunks) {
            if (!chunk) continue;
            for (size_t index = 0U; index < chunk->count; ++index) {
                if (chunk->entities[index] != entity.ecs) continue;
                const Transform3* current = scene_.GetTransform(entity.scene);
                if (!current || !std::isfinite(chunk->posX[index]) || !std::isfinite(chunk->posZ[index])) {
                    lastError_ = CanonicalWorldError::PhysicsReadbackFailed;
                    return false;
                }
                Transform3 next = *current;
                next.x = chunk->posX[index];
                next.z = chunk->posZ[index];
                if (!scene_.SetTransform(entity.scene, next)) {
                    lastError_ = CanonicalWorldError::PhysicsReadbackFailed;
                    return false;
                }
                found = true;
                break;
            }
            if (found) break;
        }
        if (!found) { lastError_ = CanonicalWorldError::PhysicsReadbackFailed; return false; }
    }
    return true;
}

bool CanonicalRuntimeWorld::Raycast(const GameplayRay2& ray, GameplayRayHit2& hit) {
    if (physicsQuery_.Raycast(physics_, ray, hit)) { lastError_ = CanonicalWorldError::None; return true; }
    lastError_ = CanonicalWorldError::QueryFailed;
    return false;
}

bool CanonicalRuntimeWorld::RaycastSet(const std::vector<GameplayRay2>& rays, std::vector<GameplayRayHit2>& hits) {
    if (physicsQuery_.RaycastSet(physics_, rays, hits)) { lastError_ = CanonicalWorldError::None; return true; }
    lastError_ = CanonicalWorldError::QueryFailed;
    return false;
}

bool CanonicalRuntimeWorld::OverlapCircle(const GameplayOverlapCircle2& circle, std::vector<EntityID>& entities) {
    if (physicsQuery_.OverlapCircle(physics_, circle, entities)) { lastError_ = CanonicalWorldError::None; return true; }
    lastError_ = CanonicalWorldError::QueryFailed;
    return false;
}

bool CanonicalRuntimeWorld::OverlapCircleSet(const std::vector<GameplayOverlapCircle2>& circles,
                                             std::vector<std::vector<EntityID>>& entitySets) {
    if (physicsQuery_.OverlapCircleSet(physics_, circles, entitySets)) { lastError_ = CanonicalWorldError::None; return true; }
    lastError_ = CanonicalWorldError::QueryFailed;
    return false;
}

bool CanonicalRuntimeWorld::ConfigureTrigger(uint8_t triggerIndex, GameplayTriggerCircleConfig config) {
    if (triggerIndex >= kMaxTriggers) { lastError_ = CanonicalWorldError::Capacity; return false; }
    if (!triggers_[triggerIndex].Initialize(config)) { lastError_ = CanonicalWorldError::QueryFailed; return false; }
    triggerConfigured_[triggerIndex] = true;
    lastError_ = CanonicalWorldError::None;
    return true;
}

bool CanonicalRuntimeWorld::UpdateTrigger(uint8_t triggerIndex) {
    if (triggerIndex >= kMaxTriggers || !triggerConfigured_[triggerIndex]) {
        lastError_ = CanonicalWorldError::InvalidEntity;
        return false;
    }
    if (!triggers_[triggerIndex].Update(physics_)) {
        lastError_ = CanonicalWorldError::TriggerUpdateFailed;
        return false;
    }
    lastError_ = CanonicalWorldError::None;
    return true;
}

const GameplayTriggerDelta* CanonicalRuntimeWorld::TriggerDelta(uint8_t triggerIndex) const {
    if (triggerIndex >= kMaxTriggers || !triggerConfigured_[triggerIndex]) return nullptr;
    return &triggers_[triggerIndex].LastDelta();
}

bool CanonicalRuntimeWorld::IsPhysicsEntityAwake(const CanonicalEntity& entity) const {
    if (!ValidateEntity(entity) || !entity.hasECS || !IsPhysicsBody(entity.componentMask)) return false;
    return physics_.IsEntityAwake(entity.ecs);
}

bool CanonicalRuntimeWorld::WakePhysicsEntity(const CanonicalEntity& entity) {
    if (!ValidateEntity(entity) || !entity.hasECS || !IsPhysicsBody(entity.componentMask)) {
        lastError_ = CanonicalWorldError::InvalidEntity; return false;
    }
    if (!physics_.WakeEntity(entity.ecs)) {
        lastError_ = CanonicalWorldError::PhysicsSyncFailed; return false;
    }
    lastError_ = CanonicalWorldError::None; return true;
}

bool CanonicalRuntimeWorld::SleepPhysicsEntity(const CanonicalEntity& entity) {
    if (!ValidateEntity(entity) || !entity.hasECS || !IsPhysicsBody(entity.componentMask)) {
        lastError_ = CanonicalWorldError::InvalidEntity; return false;
    }
    if (!physics_.SleepEntity(entity.ecs)) {
        lastError_ = CanonicalWorldError::PhysicsSyncFailed; return false;
    }
    lastError_ = CanonicalWorldError::None; return true;
}

bool CanonicalRuntimeWorld::WakePhysicsEntities(const std::vector<CanonicalEntity>& entities) {
    if (entities.empty()) { lastError_ = CanonicalWorldError::InvalidEntity; return false; }
    for (const CanonicalEntity& entity : entities) {
        if (!ValidateEntity(entity) || !entity.hasECS || !IsPhysicsBody(entity.componentMask)) {
            lastError_ = CanonicalWorldError::InvalidEntity; return false;
        }
    }
    std::vector<CanonicalEntity> changed;
    changed.reserve(entities.size());
    for (const CanonicalEntity& entity : entities) {
        if (physics_.IsEntityAwake(entity.ecs)) continue;
        if (!physics_.WakeEntity(entity.ecs)) {
            for (const CanonicalEntity& rollback : changed) (void)physics_.SleepEntity(rollback.ecs);
            lastError_ = CanonicalWorldError::PhysicsSyncFailed;
            return false;
        }
        changed.push_back(entity);
    }
    lastError_ = CanonicalWorldError::None;
    return true;
}

bool CanonicalRuntimeWorld::Step(float dt) {
    lastFrame_.physicsStepped = false;
    if (!std::isfinite(dt) || dt <= 0.0F || dt > 0.25F) { lastError_ = CanonicalWorldError::InvalidDeltaTime; return false; }
    if (bindingCount_ > kMaxBindings) { lastError_ = CanonicalWorldError::Capacity; return false; }
    if (!SyncSceneToPhysics()) return false;

    physics_.Step(ecs_, dt);
    if (!ReadBackPhysicsToScene()) return false;

    for (uint8_t triggerIndex = 0U; triggerIndex < kMaxTriggers; ++triggerIndex) {
        if (!triggerConfigured_[triggerIndex]) continue;
        if (!triggers_[triggerIndex].Update(physics_)) {
            lastError_ = CanonicalWorldError::TriggerUpdateFailed;
            return false;
        }
    }

    ++frame_;
    lastFrame_.frame = frame_;
    lastFrame_.sceneEntities = scene_.AliveCount();
    lastFrame_.physicsEntities = 0U;
    for (uint16_t i = 0U; i < bindingCount_; ++i)
        if (bindings_[i].entity.active && bindings_[i].entity.hasECS &&
            IsPhysicsBody(bindings_[i].entity.componentMask)) ++lastFrame_.physicsEntities;
    lastFrame_.physicsManifolds = physics_.GetManifoldCount();
    lastFrame_.physicsContacts = physics_.GetBroadphaseStats().candidatePairs;
    lastFrame_.physicsRevision = ecs_.GetPhysicsRevision();
    lastFrame_.physicsStepped = true;
    lastError_ = CanonicalWorldError::None;
    return true;
}

bool CanonicalRuntimeWorld::RenderSoftware(RenderCamera& camera, SoftwareRenderer& renderer,
                                            const DirectionalLight& light) {
    if (!std::isfinite(light.direction.x) || !std::isfinite(light.direction.y) || !std::isfinite(light.direction.z) ||
        !std::isfinite(light.color.x) || !std::isfinite(light.color.y) || !std::isfinite(light.color.z) ||
        !std::isfinite(light.intensity) || light.intensity < 0.0F) {
        lastError_ = CanonicalWorldError::RenderFailed; return false;
    }
    if (!rendererAdapter_.Draw(scene_, meshes_, sprites_, camera, renderer, light)) {
        lastError_ = CanonicalWorldError::RenderFailed; return false;
    }
    lastError_ = CanonicalWorldError::None;
    return true;
}

bool CanonicalRuntimeWorld::RenderVulkan3D(RenderCamera& camera, Vulkan3DRenderer& renderer,
                                            float clearR, float clearG, float clearB, float clearA) {
    if (!std::isfinite(clearR) || !std::isfinite(clearG) || !std::isfinite(clearB) || !std::isfinite(clearA) ||
        clearR < 0.0F || clearR > 1.0F || clearG < 0.0F || clearG > 1.0F || clearB < 0.0F || clearB > 1.0F || clearA < 0.0F || clearA > 1.0F) {
        lastError_ = CanonicalWorldError::RenderFailed; return false;
    }
    if (!rendererAdapter_.DrawVulkan3D(scene_, meshes_, camera, renderer, clearR, clearG, clearB, clearA)) {
        lastError_ = CanonicalWorldError::RenderFailed; return false;
    }
    lastError_ = CanonicalWorldError::None;
    return true;
}

} // namespace NeoEngine
