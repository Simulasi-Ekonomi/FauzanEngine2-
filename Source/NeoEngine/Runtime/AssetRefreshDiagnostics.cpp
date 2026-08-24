#include "AssetRefreshDiagnostics.h"

#include <algorithm>

namespace NeoEngine {
namespace {
bool Append(std::vector<AssetRefreshPlanEntry>& entries, AssetRefreshPlanEntry entry) {
    if (entries.size() >= AssetRefreshDiagnostics::kMaxEntries) return false;
    entries.push_back(std::move(entry));
    return true;
}

bool HasRebind(const std::vector<AssetRefreshPlanEntry>& entries, SceneEntity entity) {
    return std::any_of(entries.begin(), entries.end(), [entity](const AssetRefreshPlanEntry& entry) { return entry.action == AssetRefreshAction::RebindSceneInstance && entry.entity == entity; });
}
} // namespace

bool AssetRefreshDiagnostics::BuildPlan(const AssetRegistry& registry, std::string_view changedId, const TextureStagingStore& textures, const MeshStagingStore& meshes, const MaterialStagingStore& materials, const SceneMeshAdapter& scene) {
    AssetReloadDiagnostics dependencyPlan;
    if (!dependencyPlan.BuildPlan(registry, changedId)) {
        lastError_ = dependencyPlan.LastError() == AssetReloadDiagnosticsError::MissingAsset ? AssetRefreshDiagnosticsError::MissingAsset : AssetRefreshDiagnosticsError::DependencyPlanFailed;
        return false;
    }

    std::vector<AssetRefreshPlanEntry> candidates;
    candidates.reserve(kMaxEntries);
    for (const std::string& affectedId : dependencyPlan.AffectedIds()) {
        const AssetDefinition* affectedDefinition = registry.Find(affectedId);
        if (affectedDefinition == nullptr || affectedDefinition->state != AssetState::Ready) { lastError_ = AssetRefreshDiagnosticsError::DependencyPlanFailed; return false; }
        const uint64_t expectedHash = affectedDefinition->contentHash;
        for (const CpuTextureResource& resource : textures.Resources()) {
            if (resource.assetId == affectedId && !textures.IsCurrent(registry, affectedId) && !Append(candidates, {AssetRefreshAction::RefreshTexture, affectedId, {}, {}, expectedHash})) { lastError_ = AssetRefreshDiagnosticsError::Capacity; return false; }
        }
        for (const CpuMeshResource& resource : meshes.Resources()) {
            if (resource.assetId == affectedId && !meshes.IsCurrent(registry, affectedId) && !Append(candidates, {AssetRefreshAction::RefreshMesh, affectedId, {}, {}, expectedHash})) { lastError_ = AssetRefreshDiagnosticsError::Capacity; return false; }
        }
        for (const CpuMaterialResource& resource : materials.Resources()) {
            if (resource.assetId == affectedId && !materials.IsCurrent(registry, affectedId, resource.materialName) && !Append(candidates, {AssetRefreshAction::RefreshMaterial, affectedId, resource.materialName, {}, expectedHash})) { lastError_ = AssetRefreshDiagnosticsError::Capacity; return false; }
        }
        for (const SceneMeshInstance& instance : scene.Instances()) {
            const CpuMeshResource* mesh = meshes.Find(instance.sourceAssetId);
            const CpuMaterialResource* material = instance.sourceMaterialAssetId.empty() ? nullptr : materials.Find(instance.sourceMaterialAssetId, instance.sourceMaterialName);
            const CpuTextureResource* texture = instance.sourceTextureAssetId.empty() ? nullptr : textures.Find(instance.sourceTextureAssetId);
            const bool meshNeedsRebind = instance.sourceAssetId == affectedId && mesh != nullptr && (!meshes.IsCurrent(registry, instance.sourceAssetId) || instance.sourceHash != mesh->sourceHash);
            const bool materialNeedsRebind = instance.sourceMaterialAssetId == affectedId && material != nullptr && (!materials.IsCurrent(registry, instance.sourceMaterialAssetId, instance.sourceMaterialName) || instance.sourceMaterialHash != material->sourceHash);
            const bool textureNeedsRebind = instance.sourceTextureAssetId == affectedId && texture != nullptr && (!textures.IsCurrent(registry, instance.sourceTextureAssetId) || instance.sourceTextureHash != texture->sourceHash);
            if ((meshNeedsRebind || materialNeedsRebind || textureNeedsRebind) && !HasRebind(candidates, instance.entity) && !Append(candidates, {AssetRefreshAction::RebindSceneInstance, affectedId, {}, instance.entity, expectedHash})) { lastError_ = AssetRefreshDiagnosticsError::Capacity; return false; }
        }
    }
    entries_ = std::move(candidates);
    lastError_ = AssetRefreshDiagnosticsError::None;
    return true;
}

} // namespace NeoEngine
