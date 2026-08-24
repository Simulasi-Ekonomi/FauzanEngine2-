#include "Runtime/SceneWorld.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    FarmSystem farm(20, 20, 100);
    TrustSafetySystem trust;
    FarmWorldTool world;
    FarmWorldConfig config{};
    config.npcCount = 5;
    if (!world.Initialize(farm, trust, "scene-sync-player", config)) return 1;
    SceneWorld scene;
    if (!world.PopulateScene(scene) || scene.AliveCount() != 6) return 1;
    const FarmWorldNpc* firstNpc = world.Npc(1);
    const SceneEntity* firstNpcEntity = world.NpcSceneEntity(1);
    if (firstNpc == nullptr || firstNpcEntity == nullptr || scene.GetTransform(*firstNpcEntity) == nullptr) return 1;
    const Transform3 beforeNpc = *scene.GetTransform(*firstNpcEntity);
    if (!world.Tick(1) || world.Npc(1) == nullptr || scene.GetTransform(*firstNpcEntity) == nullptr) return 1;
    const Transform3 afterNpc = *scene.GetTransform(*firstNpcEntity);
    if (afterNpc.x != world.Npc(1)->x || afterNpc.z != world.Npc(1)->z || (afterNpc.x == beforeNpc.x && afterNpc.z == beforeNpc.z)) return 1;
    if (!world.SetCharacterState({7, 8, 2}) || !world.SyncScene() || world.CharacterSceneEntity() == nullptr || scene.GetTransform(*world.CharacterSceneEntity()) == nullptr) return 1;
    const Transform3 character = *scene.GetTransform(*world.CharacterSceneEntity());
    if (character.x != 7.0F || character.z != 8.0F) return 1;
    uint64_t permit = 0;
    uint32_t building = 0;
    if (!world.SetGovernmentPolicy(FarmGovernmentPolicy::ConstructionPermits, true) || !world.IssueBuildingPermit(FarmBuildingType::Barn, permit) || !world.PlaceBuilding(permit, 5, 6, building) || world.BuildingSceneEntity(building) == nullptr || scene.AliveCount() != 7 || !world.RemoveBuilding(building) || world.BuildingSceneEntity(building) != nullptr || scene.AliveCount() != 6) return 1;
    std::printf("FARM_SCENE_SYNC_SMOKE_OK npcs=5 tickSync=1 characterSync=1 buildingLifecycle=1\n");
    return 0;
}
