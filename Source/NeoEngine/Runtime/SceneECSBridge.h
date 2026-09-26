#pragma once
#include "Core/ECS/ArchetypeManager.h"
#include "SceneWorld.h"
#include <cstdint>
#include <unordered_map>
namespace NeoEngine {
struct SceneECSBridgeReceipt { uint32_t sceneCount=0; uint32_t ecsCount=0; uint64_t revision=0; };
class SceneECSBridge {
public:
 bool Rebuild(SceneWorld& scene, ArchetypeManager& ecs);
 const SceneECSBridgeReceipt& LastReceipt() const { return receipt_; }
 EntityID ECSId(SceneEntity entity) const;
private:
 std::unordered_map<uint32_t, EntityID> map_;
 SceneECSBridgeReceipt receipt_{};
};
}
