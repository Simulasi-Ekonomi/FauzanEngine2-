#pragma once

#include "AssetReloadDiagnostics.h"
#include "MaterialStaging.h"
#include "MeshStaging.h"
#include "SceneMeshAdapter.h"
#include "SceneSpriteAdapter.h"
#include "TextureStaging.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {

enum class AssetRefreshDiagnosticsError : uint8_t { None, MissingAsset, DependencyPlanFailed, Capacity };
enum class AssetRefreshAction : uint8_t { RefreshTexture, RefreshMesh, RefreshMaterial, RebindSceneInstance, RefreshSpriteInstance };

struct AssetRefreshPlanEntry {
    AssetRefreshAction action = AssetRefreshAction::RefreshTexture;
    std::string assetId;
    std::string materialName;
    SceneEntity entity{};
    uint64_t expectedHash = 0U;
};

class AssetRefreshDiagnostics {
public:
    static constexpr size_t kMaxEntries = TextureStagingStore::kMaxTextures + MeshStagingStore::kMaxMeshes + MaterialStagingStore::kMaxMaterials + SceneMeshAdapter::kMaxInstances + SceneSpriteAdapter::kMaxInstances;

    bool BuildPlan(const AssetRegistry& registry, std::string_view changedId, const TextureStagingStore& textures, const MeshStagingStore& meshes, const MaterialStagingStore& materials, const SceneMeshAdapter& scene);
    // Extends the mesh-only plan with stale sprite bindings in adapter insertion
    // order. The mesh-only overload remains unchanged for existing callers.
    bool BuildPlan(const AssetRegistry& registry, std::string_view changedId, const TextureStagingStore& textures, const MeshStagingStore& meshes, const MaterialStagingStore& materials, const SceneMeshAdapter& scene, const SceneSpriteAdapter& sprites);
    [[nodiscard]] const std::vector<AssetRefreshPlanEntry>& Entries() const { return entries_; }
    [[nodiscard]] AssetRefreshDiagnosticsError LastError() const { return lastError_; }

private:
    bool BuildPlanImpl(const AssetRegistry& registry, std::string_view changedId, const TextureStagingStore& textures, const MeshStagingStore& meshes, const MaterialStagingStore& materials, const SceneMeshAdapter& scene, const SceneSpriteAdapter* sprites);
    std::vector<AssetRefreshPlanEntry> entries_;
    AssetRefreshDiagnosticsError lastError_ = AssetRefreshDiagnosticsError::None;
};

} // namespace NeoEngine
