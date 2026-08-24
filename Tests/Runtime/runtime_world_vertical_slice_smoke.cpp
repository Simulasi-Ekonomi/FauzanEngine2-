#include "Runtime/NeoRuntime.h"

#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    NeoRuntime runtime;
    if (!runtime.Initialize({}) || runtime.Authoring() == nullptr || runtime.AuthoringWorld() == nullptr || runtime.Scene() == nullptr || runtime.AuthoringWorld()->TreeCount() == 0) return 1;
    const GridNavigation& navigation = runtime.AuthoringWorld()->Navigation();
    GridCell start{}, goal{}; std::vector<GridCell> route; bool found = false;
    for (uint16_t z = 0; z < navigation.Side() && !found; ++z) for (uint16_t x = 0; x < navigation.Side() && !found; ++x) for (uint16_t gz = 0; gz < navigation.Side() && !found; ++gz) for (uint16_t gx = 0; gx < navigation.Side() && !found; ++gx) if (!navigation.IsBlocked({x, z}) && !navigation.IsBlocked({gx, gz}) && navigation.FindPath({x, z}, {gx, gz}, route) && route.size() > 2) { start = {x, z}; goal = {gx, gz}; found = true; }
    if (!found) return 1;
    GridCell buildingCell{}; bool buildingFound = false; for (uint16_t z = 0; z < navigation.Side() && !buildingFound; ++z) for (uint16_t x = 0; x < navigation.Side() && !buildingFound; ++x) if (!navigation.IsBlocked({x, z}) && std::find(route.begin(), route.end(), GridCell{x, z}) == route.end()) { buildingCell = {x, z}; buildingFound = true; }
    if (!buildingFound || !runtime.AuthoringWorld()->PlaceBuilding({30, buildingCell.x, buildingCell.z, 1, 1}) || runtime.AuthoringWorld()->BuildingCount() != 1) return 1;
    AuthoringCatalog& catalog = *runtime.Authoring();
    if (!catalog.AddMaterial({1, 600, 350, 100, 850}) || !catalog.AddSkeleton({10, {{1, -1, 0}, {2, 0, 320}}}) || !catalog.AddCharacter({20, 10, 1, 100, 90}) || !catalog.AddBuilding({30, 1, 1, 1, 500}) || !catalog.AddItem({40, AuthoringItemClass::Equipment, 1, 1, 1000}) || !catalog.AddNarrative({51, "slice-end", {}}) || !catalog.AddNarrative({50, "slice-start", {51}}) || !catalog.AddActor({60, AuthoringActorKind::Character, 20, 1, AuthoringBehavior::Idle, 1}) || !catalog.AddActor({61, AuthoringActorKind::Npc, 20, 1, AuthoringBehavior::Patrol, 1}) || !catalog.AddActor({62, AuthoringActorKind::Monster, 20, 1, AuthoringBehavior::ChaseOrigin, 1}) || !catalog.AddScene({70, {{AuthoringSceneObjectKind::Actor, 60, static_cast<int16_t>(start.x), static_cast<int16_t>(start.z)}, {AuthoringSceneObjectKind::Actor, 61, static_cast<int16_t>(start.x), static_cast<int16_t>(start.z)}, {AuthoringSceneObjectKind::Actor, 62, static_cast<int16_t>(goal.x), static_cast<int16_t>(goal.z)}}}) || !catalog.BindScene(70, *runtime.Scene(), 128) || !catalog.SetActorGoal(60, goal, navigation)) return 1;
    for (size_t tick = 0; tick < route.size() + 2; ++tick) if (!runtime.Tick()) return 1;
    const SceneEntity* character = catalog.BoundEntity(60);
    const uint32_t treeCount = runtime.AuthoringWorld()->TreeCount();
    if (character == nullptr || runtime.Scene()->GetTransform(*character) == nullptr || runtime.Scene()->GetTransform(*character)->x != static_cast<float>(goal.x) || runtime.Scene()->GetTransform(*character)->z != static_cast<float>(goal.z) || !runtime.Shutdown()) return 1;
    std::printf("RUNTIME_WORLD_VERTICAL_SLICE_SMOKE_OK trees=%u buildings=1 route=%zu characterGoal=1 npc=1 monster=1 narrative=1 item=1\n", treeCount, route.size());
    return 0;
}
