#pragma once

#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Runtime/AssetRegistry.h"
#include "Runtime/AssetResourceManager.h"
#include "Runtime/SceneMeshAdapter.h"
#include "Runtime/SceneRenderAdapter.h"
#include "Runtime/SceneSpriteAdapter.h"
#include "Runtime/SceneWorld.h"
#include "Runtime/GameplayPhysicsQuery.h"
#include "Runtime/GameplayTriggerTracker.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace NeoEngine {

enum class CanonicalTransformAuthority : uint8_t { Scene, Physics };

enum class CanonicalWorldError : uint8_t {
    None, Capacity, InvalidTransform, InvalidEntity, PhysicsCreationFailed,
    PhysicsSyncFailed, PhysicsStepFailed, PhysicsReadbackFailed,
    RenderFailed, MeshBindingFailed, QueryFailed, TransformAuthorityViolation,
    TriggerUpdateFailed
};

struct CanonicalEntity {
    SceneEntity scene{};
    EntityID ecs = 0xFFFFFFFFU;
    uint32_t componentMask = 0U;
    CanonicalTransformAuthority authority = CanonicalTransformAuthority::Scene;
    bool hasECS = false;
    bool active = false;
};

struct CanonicalFrameReceipt {
    uint64_t frame = 0U;
    uint32_t sceneEntities = 0U;
    uint32_t physicsEntities = 0U;
    size_t physicsManifolds = 0U;
    size_t physicsContacts = 0U;
    uint64_t physicsRevision = 0U;
    bool physicsStepped = false;
};

class CanonicalRuntimeWorld {
public:
    static constexpr uint16_t kMaxEntities = SceneWorld::kCapacity;
    static constexpr uint8_t kMaxTriggers = 64U;

    CanonicalRuntimeWorld();
    CanonicalRuntimeWorld(const CanonicalRuntimeWorld&) = delete;
    CanonicalRuntimeWorld& operator=(const CanonicalRuntimeWorld&) = delete;

    bool CreateEntity(const Transform3& transform, uint32_t componentMask,
                      CanonicalTransformAuthority authority, CanonicalEntity& outEntity);
    bool DestroyEntity(CanonicalEntity entity);
    bool SetTransform(CanonicalEntity entity, const Transform3& transform);
    bool BindMesh(const SceneMeshInstance& instance);
    bool Raycast(const GameplayRay2& ray, GameplayRayHit2& hit);
    bool RaycastSet(const std::vector<GameplayRay2>& rays, std::vector<GameplayRayHit2>& hits);
    bool OverlapCircle(const GameplayOverlapCircle2& circle, std::vector<EntityID>& entities);
    bool OverlapCircleSet(const std::vector<GameplayOverlapCircle2>& circles, std::vector<std::vector<EntityID>>& entitySets);
    bool ConfigureTrigger(uint8_t triggerIndex, GameplayTriggerCircleConfig config);
    bool UpdateTrigger(uint8_t triggerIndex);
    [[nodiscard]] const GameplayTriggerDelta* TriggerDelta(uint8_t triggerIndex) const;
    bool IsPhysicsEntityAwake(const CanonicalEntity& entity) const;
    bool WakePhysicsEntity(const CanonicalEntity& entity);
    bool SleepPhysicsEntity(const CanonicalEntity& entity);
    bool WakePhysicsEntities(const std::vector<CanonicalEntity>& entities);
    [[nodiscard]] bool GetEntity(SceneEntity sceneEntity, CanonicalEntity& outEntity) const;

    bool Step(float dt);
    bool RenderSoftware(RenderCamera& camera, SoftwareRenderer& renderer,
                        const DirectionalLight& light);
    bool RenderVulkan3D(RenderCamera& camera, Vulkan3DRenderer& renderer,
                        float clearR, float clearG, float clearB, float clearA);

    [[nodiscard]] const SceneWorld& Scene() const { return scene_; }
    [[nodiscard]] SceneWorld& Scene() { return scene_; }
    [[nodiscard]] const ArchetypeManager& ECS() const { return ecs_; }
    [[nodiscard]] ArchetypeManager& ECS() { return ecs_; }
    [[nodiscard]] const XPBDPhysicsSystem& Physics() const { return physics_; }
    [[nodiscard]] XPBDPhysicsSystem& Physics() { return physics_; }
    [[nodiscard]] const AssetRegistry& Assets() const { return assets_; }
    [[nodiscard]] AssetRegistry& Assets() { return assets_; }
    [[nodiscard]] const AssetResourceManager& Resources() const { return resources_; }
    [[nodiscard]] AssetResourceManager& Resources() { return resources_; }
    [[nodiscard]] const CanonicalFrameReceipt& LastFrame() const { return lastFrame_; }
    [[nodiscard]] CanonicalWorldError LastError() const { return lastError_; }

private:
    struct Binding { CanonicalEntity entity{}; };
    static constexpr size_t kMaxBindings = kMaxEntities;

    bool ValidateTransform(const Transform3& transform) const;
    bool ValidateEntity(const CanonicalEntity& entity) const;
    bool SyncSceneToPhysics();
    bool ReadBackPhysicsToScene();
    static bool IsPhysicsBody(uint32_t componentMask);

    SceneWorld scene_;
    ArchetypeManager ecs_;
    XPBDPhysicsSystem physics_;
    AssetRegistry assets_;
    AssetResourceManager resources_;
    SceneMeshAdapter meshes_;
    SceneSpriteAdapter sprites_;
    SceneRenderAdapter rendererAdapter_;
    std::array<Binding, kMaxBindings> bindings_{};
    std::array<GameplayTriggerTracker, kMaxTriggers> triggers_{};
    std::array<bool, kMaxTriggers> triggerConfigured_{};
    uint16_t bindingCount_ = 0U;
    uint64_t frame_ = 0U;
    CanonicalFrameReceipt lastFrame_{};
    GameplayPhysicsQuery physicsQuery_{};
    CanonicalWorldError lastError_ = CanonicalWorldError::None;
};

} // namespace NeoEngine
