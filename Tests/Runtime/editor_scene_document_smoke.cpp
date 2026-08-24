#include "Runtime/EditorSceneDocument.h"

#include <cmath>
#include <cstdio>
#include <limits>

int main() {
    using namespace NeoEngine;
    AssetRegistry assets;
    if (!assets.ImportBytes("mesh.cube", AssetKind::Mesh, {}, {1, 2, 3}) || !assets.MarkReady("mesh.cube") || !assets.ImportBytes("material.grass", AssetKind::Material, {}, {1, 2, 3}) || !assets.MarkReady("material.grass") || !assets.ImportBytes("texture.grass", AssetKind::Texture, {}, {1, 2, 3}) || !assets.MarkReady("texture.grass")) return 1;
    EditorSceneDocument document{EditorSceneDocument::kVersion, "farm-slice", 1, {
        {10, 0, EditorSceneActorKind::Mesh, {4, 0, 0, 0, 0, 0, 1, 1, 1}, "mesh.cube", "material.grass", "grass", "texture.grass"},
        {20, 10, EditorSceneActorKind::Marker, {2, 0, 0, 0, 0, 0, 1, 1, 1}, ""},
    }};
    SceneWorld target;
    EditorSceneDocumentAdapter adapter;
    if (!adapter.Load(document, assets, target) || target.AliveCount() != 2 || adapter.LastError() != EditorSceneDocumentError::None) return 1;
    const SceneEntity* child = adapter.EntityForActor(20);
    if (child == nullptr || target.GetTransform(*child) == nullptr || std::fabs(target.GetTransform(*child)->x - 6.0F) > 0.0001F) return 1;

    const uint32_t preservedCount = target.AliveCount();
    const SceneEntity preservedChild = *child;
    EditorSceneDocument missingAsset = document;
    missingAsset.revision = 2;
    missingAsset.actors[0].assetId = "mesh.missing";
    if (adapter.Load(missingAsset, assets, target) || adapter.LastError() != EditorSceneDocumentError::MissingAsset || target.AliveCount() != preservedCount || target.GetTransform(preservedChild) == nullptr) return 1;

    EditorSceneDocument invalidTransform = document;
    invalidTransform.revision = 3;
    invalidTransform.actors[1].transform.sx = std::numeric_limits<float>::infinity();
    if (adapter.Load(invalidTransform, assets, target) || adapter.LastError() != EditorSceneDocumentError::InvalidActor || target.AliveCount() != preservedCount) return 1;

    EditorSceneDocument cycle = document;
    cycle.revision = 4;
    cycle.actors[0].parentId = 20;
    if (adapter.Load(cycle, assets, target) || adapter.LastError() != EditorSceneDocumentError::InvalidHierarchy || target.AliveCount() != preservedCount) return 1;

    EditorSceneDocument missingMaterial = document;
    missingMaterial.revision = 5;
    missingMaterial.actors[0].materialAssetId = "material.missing";
    if (adapter.Load(missingMaterial, assets, target) || adapter.LastError() != EditorSceneDocumentError::MissingMaterial || target.AliveCount() != preservedCount) return 1;

    std::printf("EDITOR_SCENE_DOCUMENT_SMOKE_OK actors=%u atomic=1 assets=1 hierarchy=1\n", target.AliveCount());
    return 0;
}
