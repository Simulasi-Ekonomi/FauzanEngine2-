#pragma once

#include "Core/ECS/ArchetypeManager.h"
#include "SceneWorld.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {
enum class ScenePhysicsPoseSyncError : uint8_t { None, Capacity, InvalidSceneEntity, DuplicateSceneEntity, DuplicatePhysicsEntity, MissingWorldTransform, MissingPhysicsPosition, InvalidTransform };

// One-way scene-to-physics pose mirror. It never writes SceneWorld, runs XPBD, or
// controls movement authority. All candidate poses are validated before ECS writes begin.
class ScenePhysicsPoseSync {
public:
    static constexpr size_t kMaxBindings = 1024;
    bool Bind(SceneEntity sceneEntity, EntityID physicsEntity);
    bool Sync(const SceneWorld& world, ArchetypeManager& entities);
    [[nodiscard]] size_t BindingCount() const { return bindings_.size(); }
    [[nodiscard]] ScenePhysicsPoseSyncError LastError() const { return lastError_; }
private:
    struct Binding { SceneEntity scene{}; EntityID physics = 0; };
    std::vector<Binding> bindings_;
    ScenePhysicsPoseSyncError lastError_ = ScenePhysicsPoseSyncError::None;
};
} // namespace NeoEngine
