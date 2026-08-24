#include "Systems/AuthoringCatalog.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    AuthoringCatalog catalog;
    if (!catalog.AddMaterial({1, 800, 400, 100, 900}) || !catalog.AddMaterial({2, 300, 700, 50, 500})) return 1;
    if (!catalog.AddSkeleton({10, {{1, -1, 0}, {2, 0, 450}, {3, 1, 320}}})) return 1;
    if (catalog.AddSkeleton({11, {{1, 1, 0}}}) || catalog.LastError() != AuthoringError::InvalidHierarchy) return 1;
    if (!catalog.AddCharacter({20, 10, 1, 100, 80}) || !catalog.AddBuilding({30, 2, 2, 3, 600}) || !catalog.AddItem({40, AuthoringItemClass::Equipment, 1, 1, 2400})) return 1;
    if (!catalog.AddNarrative({51, "farm-ending", {}}) || !catalog.AddNarrative({50, "farm-intro", {51}})) return 1;
    if (!catalog.AddActor({59, AuthoringActorKind::Character, 20, 1, AuthoringBehavior::Idle, 1}) || !catalog.AddActor({60, AuthoringActorKind::Npc, 20, 1, AuthoringBehavior::Patrol, 2}) || !catalog.AddActor({61, AuthoringActorKind::Monster, 20, 2, AuthoringBehavior::ChaseOrigin, 1})) return 1;
    if (!catalog.AddScene({70, {{AuthoringSceneObjectKind::Building, 30, 5, 5}, {AuthoringSceneObjectKind::Actor, 59, 1, 1}, {AuthoringSceneObjectKind::Actor, 60, -2, 0}, {AuthoringSceneObjectKind::Actor, 61, 3, 3}}})) return 1;
    ContactMaterialResponse contact{};
    if (!catalog.Contact(1, 2, contact) || contact.hardnessPermille != 300 || contact.frictionPermille != 550 || contact.restitutionPermille != 50) return 1;
    SceneWorld scene;
    if (!catalog.BindScene(70, scene, 8) || catalog.BoundEntityCount() != 4 || catalog.BoundEntity(60) == nullptr) return 1;
    GridNavigation navigation; if (!navigation.Initialize(8) || !navigation.SetBlocked({2, 1}, true) || !catalog.SetActorGoal(59, {4, 1}, navigation)) return 1;
    const Transform3 before = *scene.GetTransform(*catalog.BoundEntity(60));
    if (!catalog.Tick(2) || scene.GetTransform(*catalog.BoundEntity(60)) == nullptr) return 1;
    const Transform3 after = *scene.GetTransform(*catalog.BoundEntity(60));
    if (before.x == after.x && before.z == after.z) return 1;
    if (!catalog.Tick(8) || scene.GetTransform(*catalog.BoundEntity(59)) == nullptr || scene.GetTransform(*catalog.BoundEntity(59))->x != 4.0F || scene.GetTransform(*catalog.BoundEntity(59))->z != 1.0F) return 1;
    const uint64_t state = catalog.DeterministicState();
    const std::vector<uint8_t> bytes = catalog.Serialize();
    AuthoringCatalog restored;
    if (!restored.Deserialize(bytes) || restored.DeterministicState() != state || restored.Deserialize(std::span<const uint8_t>(bytes.data(), bytes.size() - 1)) || restored.LastError() != AuthoringError::CorruptPersistence) return 1;
    std::printf("AUTHORING_CATALOG_SMOKE_OK skeleton=3 materials=2 character=1 building=1 item=1 narrative=2 actors=3 scene=4 route=1 movement=1 state=%llu\n", static_cast<unsigned long long>(state));
    return 0;
}
