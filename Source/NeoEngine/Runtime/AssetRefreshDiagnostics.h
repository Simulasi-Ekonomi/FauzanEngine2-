#pragma once

#include "AssetReloadDiagnostics.h"
#include "MaterialStaging.h"
#include "MeshStaging.h"
#include "SceneMeshAdapter.h"
#include "TextureStaging.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {

enum class AssetRefreshDiagnosticsError : uint8_t { None, MissingAsset, DependencyPlanFailed, Capacity };
enum class AssetRefreshAction : uint8_t { RefreshTexture, RefreshMesh, RefreshMaterial, RebindSceneInstance };

struct AssetRefreshPlanEntry {
    AssetRefreshAction action = AssetRefreshAction::RefreshTexture;
    std::string assetId;
    std::string materialName;
    SceneEntity entity{};
    uint64_t expectedHash = 0U;
};

class AssetRefreshDiagnostics {
public:
    static constexpr size_t kMaxEntries = TextureStagingStore::kMaxTextures + MeshStagingStore::kMaxMeshes + MaterialStagingStore::kMaxMaterials + SceneMeshAdapter::kMaxInstances;

    bool BuildPlan(const AssetRegistry& registry, std::string_view changedId, const TextureStagingStore& textures, const MeshStagingStore& meshes, const MaterialStagingStore& materials, const SceneMeshAdapter& scene);
    [[nodiscard]] const std::vector<AssetRefreshPlanEntry>& Entries() const { return entries_; }
    [[nodiscard]] AssetRefreshDiagnosticsError LastError() const { return lastError_; }

private:
    std::vector<AssetRefreshPlanEntry> entries_;
    AssetRefreshDiagnosticsError lastError_ = AssetRefreshDiagnosticsError::None;
};

} // namespace NeoEngine
