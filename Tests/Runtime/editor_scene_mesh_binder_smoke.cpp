#include "Runtime/EditorSceneDocument.h"
#include "Runtime/EditorSceneMeshBinder.h"
#include "Runtime/MaterialStaging.h"
#include "Runtime/MeshStaging.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SoftwareRenderer.h"

#include <string>

int main() {
    using namespace NeoEngine;
    const auto bytes = [](const std::string& value) { return std::vector<uint8_t>(value.begin(), value.end()); };
    const std::string obj = "v -1 -1 0\nv 1 -1 0\nv 0 1 0\nvn 0 0 -1\nf 1//1 2//1 3//1\n";
    const std::string mtl = "newmtl grass\nKd 0.1 0.8 0.2\nd 1\n";
    AssetRegistry assets;
    if (!assets.ImportBytes("farm.mesh", AssetKind::Mesh, {}, bytes(obj)) || !assets.MarkReady("farm.mesh") ||
        !assets.ImportBytes("farm.material", AssetKind::Material, {}, bytes(mtl)) || !assets.MarkReady("farm.material")) return 1;

    const EditorSceneDocument document{EditorSceneDocument::kVersion, "farm_scene", 1, {{1, 0, EditorSceneActorKind::Mesh, {0, 0, 5, 0, 0, 0, 1, 1, 1}, "farm.mesh"}}};
    SceneWorld world;
    EditorSceneDocumentAdapter documentAdapter;
    if (!documentAdapter.Load(document, assets, world)) return 1;

    MeshStagingStore meshes;
    MaterialStagingStore materials;
    EditorSceneMeshBinder binder;
    SceneMeshAdapter adapter;
    const std::vector<EditorSceneMeshMaterialBinding> bindings{{1, "farm.material", "grass"}};
    if (!binder.Bind(document, documentAdapter, assets, meshes, materials, bindings, adapter) || adapter.Instances().size() != 1U ||
        adapter.Instances().front().sourceAssetId != "farm.mesh" || adapter.Instances().front().sourceMaterialAssetId != "farm.material") return 1;

    RenderCamera camera;
    SoftwareRenderer renderer;
    if (!camera.Initialize({RenderCameraMode::Perspective, {0, 0, 0}, 5, 90, 1, 0.1F, 20}) || !renderer.Initialize(64, 64) ||
        !renderer.Clear(0xFF000000U) || !adapter.Draw(world, camera, renderer, {{0, 0, -1}}) || renderer.PixelAt(32, 32) == 0xFF000000U) return 1;
    const uint64_t stableHash = renderer.FrameHash();

    if (binder.Bind(document, documentAdapter, assets, meshes, materials, {}, adapter) || binder.LastError() != EditorSceneMeshBinderError::InvalidDocument ||
        adapter.Instances().size() != 1U || adapter.Instances().front().sourceAssetId != "farm.mesh") return 1;
    const std::vector<EditorSceneMeshMaterialBinding> duplicate{{1, "farm.material", "grass"}, {1, "farm.material", "grass"}};
    if (binder.Bind(document, documentAdapter, assets, meshes, materials, duplicate, adapter) || binder.LastError() != EditorSceneMeshBinderError::DuplicateActorBinding ||
        adapter.Instances().size() != 1U) return 1;
    const std::vector<EditorSceneMeshMaterialBinding> missingMaterial{{1, "missing.material", "grass"}};
    if (binder.Bind(document, documentAdapter, assets, meshes, materials, missingMaterial, adapter) || binder.LastError() != EditorSceneMeshBinderError::MaterialStageFailed ||
        adapter.Instances().size() != 1U) return 1;

    const std::vector<uint8_t> ppm{'P', '6', '\n', '1', ' ', '1', '\n', '2', '5', '5', '\n', 255U, 0U, 0U};
    if (!assets.ImportBytes("farm.texture", AssetKind::Texture, {}, ppm) || !assets.MarkReady("farm.texture")) return 1;
    EditorSceneDocument v2 = document;
    v2.version = EditorSceneDocument::kVersion;
    v2.revision = 2;
    v2.actors[0].materialAssetId = "farm.material";
    v2.actors[0].materialName = "grass";
    v2.actors[0].textureAssetId = "farm.texture";
    if (!documentAdapter.Load(v2, assets, world)) return 1;
    TextureStagingStore textures;
    if (!binder.BindDocumentAssets(v2, documentAdapter, assets, meshes, materials, textures, adapter) || adapter.Instances().size() != 1U ||
        adapter.Instances().front().sourceMaterialAssetId != "farm.material" || adapter.Instances().front().sourceTextureAssetId != "farm.texture") return 1;
    SoftwareRenderer textured;
    if (!textured.Initialize(64, 64) || !textured.Clear(0xFF000000U) || !adapter.Draw(world, camera, textured, {{0, 0, -1}}) || textured.PixelAt(32, 32) != 0xFFFF0000U) return 1;
    const uint64_t texturedHash = textured.FrameHash();
    EditorSceneDocument missingTexture = v2;
    missingTexture.revision = 3;
    missingTexture.actors[0].textureAssetId = "missing.texture";
    if (binder.BindDocumentAssets(missingTexture, documentAdapter, assets, meshes, materials, textures, adapter) || binder.LastError() != EditorSceneMeshBinderError::TextureStageFailed || adapter.Instances().front().sourceTextureAssetId != "farm.texture") return 1;

    SoftwareRenderer repeated;
    if (!repeated.Initialize(64, 64) || !repeated.Clear(0xFF000000U) || !adapter.Draw(world, camera, repeated, {{0, 0, -1}}) || repeated.FrameHash() != texturedHash) return 1;
    return 0;
}
