#pragma once

#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Runtime/AssetRegistry.h"
#include "Runtime/AssetResourceManager.h"
#include "Runtime/SceneMeshAdapter.h"
#include "Runtime/SceneRenderAdapter.h"
#include "Runtime/SceneSpriteAdapter.h"
#include "Runtime/SceneWorld.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace NeoEngine {

enum class CanonicalTransformAuthority : uint8_t { Scene, Physics };

enum class CanonicalWorldError : uint8_t {
    None, Capacity, InvalidTransform, InvalidEntity, PhysicsCreationFailed,
    PhysicsSyncFailed, PhysicsStepFailed, PhysicsReadbackFailed,
    RenderFailed, MeshBindingFailed
};

struct CanonicalEntity {
    SceneEntity scene{};
    EntityID physics = 0xFFFFFFFFU;
    uint32_t componentMask = 0U;
    CanonicalTransformAuthority authority = CanonicalTransformAuthority::Scene;
    bool hasPhysics = false;
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

    CanonicalRuntimeWorld();
    CanonicalRuntimeWorld(const CanonicalRuntimeWorld&) = delete;
    CanonicalRuntimeWorld& operator=(const CanonicalRuntimeWorld&) = delete;

    bool CreateEntity(const Transform3& transform, uint32_t componentMask,
                      CanonicalTransformAuthority authority, CanonicalEntity& outEntity);
    bool DestroyEntity(CanonicalEntity entity);
    bool SetTransform(CanonicalEntity entity, const Transform3& transform);
    bool BindMesh(const SceneMeshInstance& instance);

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

    SceneWorld scene_;
    ArchetypeManager ecs_;
    XPBDPhysicsSystem physics_;
    AssetRegistry assets_;
    AssetResourceManager resources_;
    SceneMeshAdapter meshes_;
    SceneSpriteAdapter sprites_;
    SceneRenderAdapter rendererAdapter_;
    std::array<Binding, kMaxBindings> bindings_{};
    uint16_t bindingCount_ = 0U;
    uint64_t frame_ = 0U;
    CanonicalFrameReceipt lastFrame_{};
    CanonicalWorldError lastError_ = CanonicalWorldError::None;
};

} // namespace NeoEngine
