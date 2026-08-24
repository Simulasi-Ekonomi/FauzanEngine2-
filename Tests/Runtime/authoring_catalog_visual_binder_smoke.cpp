#include "Runtime/AuthoringCatalogVisualBinder.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SoftwareRenderer.h"

#include <string>

int main() {
    using namespace NeoEngine;
    const auto bytes = [](const std::string& value) { return std::vector<uint8_t>(value.begin(), value.end()); };
    const std::string obj = "v -1 -1 0\nv 1 -1 0\nv 0 1 0\nvn 0 0 -1\nf 1//1 2//1 3//1\n";
    const std::string mtl = "newmtl grass\nKd 0.1 0.8 0.2\nd 1\n";
    const std::vector<uint8_t> ppm{'P', '6', '\n', '1', ' ', '1', '\n', '2', '5', '5', '\n', 255U, 0U, 0U};
    AssetRegistry assets;
    if (!assets.ImportBytes("farm.mesh", AssetKind::Mesh, {}, bytes(obj)) || !assets.MarkReady("farm.mesh") || !assets.ImportBytes("farm.material", AssetKind::Material, {}, bytes(mtl)) || !assets.MarkReady("farm.material") || !assets.ImportBytes("farm.texture", AssetKind::Texture, {}, ppm) || !assets.MarkReady("farm.texture")) return 1;
    AuthoringCatalog catalog;
    if (!catalog.AddMaterial({1, 800, 400, 100, 900}) || !catalog.AddSkeleton({10, {{1, -1, 0}, {2, 0, 450}}}) || !catalog.AddCharacter({20, 10, 1, 100, 80}) || !catalog.AddBuilding({30, 1, 2, 2, 600}) || !catalog.AddActor({40, AuthoringActorKind::Npc, 20, 1, AuthoringBehavior::Patrol, 1}) || !catalog.AddScene({50, {{AuthoringSceneObjectKind::Building, 30, -2, 5}, {AuthoringSceneObjectKind::Actor, 40, 0, 5}}})) return 1;
    SceneWorld world;
    if (!catalog.BindScene(50, world, 8)) return 1;
    MeshStagingStore meshes; MaterialStagingStore materials; TextureStagingStore textures; SceneMeshAdapter adapter; AuthoringCatalogVisualBinder binder;
    const std::vector<AuthoringCatalogVisualBinding> bindings{{AuthoringSceneObjectKind::Building, 30, "farm.mesh", "farm.material", "grass", "farm.texture"}, {AuthoringSceneObjectKind::Actor, 40, "farm.mesh", "farm.material", "grass", "farm.texture"}};
    if (!binder.Bind(catalog, assets, meshes, materials, textures, bindings, adapter) || adapter.Instances().size() != 2U) return 1;
    RenderCamera camera; SoftwareRenderer renderer;
    if (!camera.Initialize({RenderCameraMode::Perspective, {0, 0, 0}, 5, 90, 1, 0.1F, 20}) || !renderer.Initialize(64, 64) || !renderer.Clear(0xFF000000U) || !adapter.Draw(world, camera, renderer, {{0, 0, -1}}) || renderer.PixelAt(32, 32) != 0xFFFF0000U) return 1;
    const uint64_t prior = renderer.FrameHash();
    const std::vector<AuthoringCatalogVisualBinding> duplicate{{AuthoringSceneObjectKind::Actor, 40, "farm.mesh", "farm.material", "grass", "farm.texture"}, {AuthoringSceneObjectKind::Actor, 40, "farm.mesh", "farm.material", "grass", "farm.texture"}};
    if (binder.Bind(catalog, assets, meshes, materials, textures, duplicate, adapter) || binder.LastError() != AuthoringCatalogVisualBinderError::DuplicateBinding || adapter.Instances().size() != 2U) return 1;
    if (!catalog.Tick(2)) return 1;
    SoftwareRenderer moved;
    if (!moved.Initialize(64, 64) || !moved.Clear(0xFF000000U) || !adapter.Draw(world, camera, moved, {{0, 0, -1}}) || moved.FrameHash() == prior) return 1;
    return 0;
}
