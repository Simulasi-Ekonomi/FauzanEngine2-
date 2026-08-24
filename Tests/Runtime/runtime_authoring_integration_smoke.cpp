#include "Runtime/NeoRuntime.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    NeoRuntime runtime;
    if (!runtime.Initialize({}) || runtime.Authoring() == nullptr || runtime.Scene() == nullptr) return 1;
    AuthoringCatalog& catalog = *runtime.Authoring();
    if (!catalog.AddMaterial({1, 700, 300, 100, 900}) || !catalog.AddSkeleton({10, {{1, -1, 0}, {2, 0, 320}}}) || !catalog.AddCharacter({20, 10, 1, 50, 50}) || !catalog.AddActor({30, AuthoringActorKind::Npc, 20, 1, AuthoringBehavior::Patrol, 1}) || !catalog.AddScene({40, {{AuthoringSceneObjectKind::Actor, 30, -3, 0}}}) || !catalog.BindScene(40, *runtime.Scene(), 8)) return 1;
    const SceneEntity* entity = catalog.BoundEntity(30);
    if (entity == nullptr || runtime.Scene()->GetTransform(*entity) == nullptr) return 1;
    const float before = runtime.Scene()->GetTransform(*entity)->x;
    if (!runtime.Tick() || runtime.Scene()->GetTransform(*entity) == nullptr || runtime.Scene()->GetTransform(*entity)->x == before || !runtime.Shutdown()) return 1;
    std::printf("RUNTIME_AUTHORING_INTEGRATION_SMOKE_OK actor=1 sceneBound=1 moved=1\n");
    return 0;
}
