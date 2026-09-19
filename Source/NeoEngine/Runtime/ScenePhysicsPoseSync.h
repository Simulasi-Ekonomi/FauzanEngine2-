#pragma once

#include "Core/ECS/ArchetypeManager.h"
#include "SceneWorld.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {
enum class ScenePhysicsPoseSyncError : uint8_t { None, Capacity, InvalidSceneEntity, DuplicateSceneEntity, DuplicatePhysicsEntity, UnknownBinding, MissingWorldTransform, MissingPhysicsPosition, InvalidTransform, InvalidPhysicsPose };

class ScenePhysicsPoseSync {
public:
    static constexpr size_t kMaxBindings = 1024;

    // Preserved compatibility: scene-authoritative binding.
    bool Bind(SceneEntity sceneEntity, EntityID physicsEntity);
    // Physics-authoritative binding for dynamic bodies.
    bool Bind(SceneEntity sceneEntity, EntityID physicsEntity, bool physicsAuthoritative);
    bool Unbind(SceneEntity sceneEntity);
    bool Sync(const SceneWorld& world, ArchetypeManager& entities);
    bool SyncFromPhysics(SceneWorld& world, ArchetypeManager& entities);
    [[nodiscard]] bool IsPhysicsAuthoritative(SceneEntity sceneEntity) const;
    [[nodiscard]] size_t BindingCount() const { return bindings_.size(); }
    [[nodiscard]] ScenePhysicsPoseSyncError LastError() const { return lastError_; }

private:
    struct Binding {
        SceneEntity scene{};
        EntityID physics = 0;
        bool physicsAuthoritative = false;
    };
    std::vector<Binding> bindings_;
    ScenePhysicsPoseSyncError lastError_ = ScenePhysicsPoseSyncError::None;
};
} // namespace NeoEngine
