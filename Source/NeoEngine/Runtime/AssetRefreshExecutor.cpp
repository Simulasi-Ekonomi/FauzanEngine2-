#include "AssetRefreshExecutor.h"

#include <algorithm>

namespace NeoEngine {
namespace {
const SceneMeshInstance* FindInstance(const SceneMeshAdapter& scene, SceneEntity entity) {
    const auto found = std::find_if(scene.Instances().begin(), scene.Instances().end(), [entity](const SceneMeshInstance& instance) { return instance.entity == entity; });
    return found == scene.Instances().end() ? nullptr : &*found;
}

bool ContainsRefresh(const std::vector<AssetRefreshPlanEntry>& prior, AssetRefreshAction action, std::string_view assetId, std::string_view materialName = {}) {
    return std::any_of(prior.begin(), prior.end(), [action, assetId, materialName](const AssetRefreshPlanEntry& entry) { return entry.action == action && entry.assetId == assetId && entry.materialName == materialName; });
}
bool MatchesExpectedHash(const AssetRegistry& registry, const AssetRefreshPlanEntry& entry) {
    const AssetDefinition* definition = registry.Find(entry.assetId);
    return entry.expectedHash != 0U && definition != nullptr && definition->state == AssetState::Ready && definition->contentHash == entry.expectedHash;
}
bool SameActionTarget(const AssetRefreshPlanEntry& left, const AssetRefreshPlanEntry& right) {
    if (left.action != right.action) return false;
    if (left.action == AssetRefreshAction::RebindSceneInstance) return left.entity == right.entity;
    return left.assetId == right.assetId && left.materialName == right.materialName;
}
bool HasDuplicateAction(const std::vector<AssetRefreshPlanEntry>& plan) {
    for (size_t index = 0U; index < plan.size(); ++index) for (size_t prior = 0U; prior < index; ++prior) if (SameActionTarget(plan[index], plan[prior])) return true;
    return false;
}
} // namespace

bool AssetRefreshExecutor::Preflight(const std::vector<AssetRefreshPlanEntry>& plan, const AssetRegistry& registry, const TextureStagingStore& textures, const MeshStagingStore& meshes, const MaterialStagingStore& materials, const SceneMeshAdapter& scene) {
    preflightReceipts_.clear();
    if (plan.size() > kMaxReceipts) { lastError_ = AssetRefreshExecutorError::Capacity; return false; }
    if (HasDuplicateAction(plan)) { lastError_ = AssetRefreshExecutorError::PlanInvalid; return false; }
    std::vector<AssetRefreshPreflightReceipt> results;
    std::vector<AssetRefreshPlanEntry> prior;
    results.reserve(plan.size());
    prior.reserve(plan.size());
    for (const AssetRefreshPlanEntry& entry : plan) {
        AssetRefreshPreflightReceipt receipt{entry.action, entry.assetId, entry.materialName, entry.entity, false};
        if (!MatchesExpectedHash(registry, entry)) { lastError_ = AssetRefreshExecutorError::PlanStale; results.push_back(std::move(receipt)); preflightReceipts_ = std::move(results); return false; }
        bool valid = false;
        switch (entry.action) {
            case AssetRefreshAction::RefreshTexture: valid = textures.Find(entry.assetId) != nullptr && !textures.IsCurrent(registry, entry.assetId) && textures.CanRefresh(registry, entry.assetId); break;
            case AssetRefreshAction::RefreshMesh: valid = meshes.Find(entry.assetId) != nullptr && !meshes.IsCurrent(registry, entry.assetId) && meshes.CanRefresh(registry, entry.assetId); break;
            case AssetRefreshAction::RefreshMaterial: valid = materials.Find(entry.assetId, entry.materialName) != nullptr && !materials.IsCurrent(registry, entry.assetId, entry.materialName) && materials.CanRefresh(registry, entry.assetId, entry.materialName); break;
            case AssetRefreshAction::RebindSceneInstance: {
                const SceneMeshInstance* instance = FindInstance(scene, entry.entity);
                if (instance == nullptr) { lastError_ = AssetRefreshExecutorError::MissingInstance; results.push_back(std::move(receipt)); preflightReceipts_ = std::move(results); return false; }
                const CpuMeshResource* mesh = meshes.Find(instance->sourceAssetId);
                const bool meshReady = mesh != nullptr && (meshes.IsCurrent(registry, instance->sourceAssetId) || ContainsRefresh(prior, AssetRefreshAction::RefreshMesh, instance->sourceAssetId));
                const CpuMaterialResource* material = instance->sourceMaterialAssetId.empty() ? nullptr : materials.Find(instance->sourceMaterialAssetId, instance->sourceMaterialName);
                const bool materialReady = instance->sourceMaterialAssetId.empty() || (material != nullptr && (materials.IsCurrent(registry, instance->sourceMaterialAssetId, instance->sourceMaterialName) || ContainsRefresh(prior, AssetRefreshAction::RefreshMaterial, instance->sourceMaterialAssetId, instance->sourceMaterialName)));
                const CpuTextureResource* texture = instance->sourceTextureAssetId.empty() ? nullptr : textures.Find(instance->sourceTextureAssetId);
                const bool textureReady = instance->sourceTextureAssetId.empty() || (texture != nullptr && (textures.IsCurrent(registry, instance->sourceTextureAssetId) || ContainsRefresh(prior, AssetRefreshAction::RefreshTexture, instance->sourceTextureAssetId)));
                if (!instance->sourceMaterialAssetId.empty()) valid = meshReady && materialReady && textureReady && scene.CanRefreshStaged(entry.entity, *mesh, *material);
                else { MeshMaterial candidate = instance->material; candidate.texture = texture; valid = meshReady && materialReady && textureReady && scene.CanRefreshStaged(entry.entity, *mesh, candidate); }
                break;
            }
        }
        receipt.structurallyValid = valid;
        results.push_back(std::move(receipt));
        if (!valid) { const bool resourceExists=entry.action==AssetRefreshAction::RefreshTexture?textures.Find(entry.assetId)!=nullptr:entry.action==AssetRefreshAction::RefreshMesh?meshes.Find(entry.assetId)!=nullptr:entry.action==AssetRefreshAction::RefreshMaterial?materials.Find(entry.assetId,entry.materialName)!=nullptr:true;lastError_=resourceExists?AssetRefreshExecutorError::ProbeFailed:AssetRefreshExecutorError::StaleResource; preflightReceipts_ = std::move(results); return false; }
        prior.push_back(entry);
    }
    preflightReceipts_ = std::move(results);
    lastError_ = AssetRefreshExecutorError::None;
    return true;
}

bool AssetRefreshExecutor::Execute(const std::vector<AssetRefreshPlanEntry>& plan, const AssetRegistry& registry, TextureStagingStore& textures, MeshStagingStore& meshes, MaterialStagingStore& materials, SceneMeshAdapter& scene) {
    receipts_.clear();
    if (!Preflight(plan, registry, textures, meshes, materials, scene)) return false;
    std::vector<AssetRefreshReceipt> results;
    results.reserve(plan.size());
    for (const AssetRefreshPlanEntry& entry : plan) {
        AssetRefreshReceipt receipt{entry.action, entry.assetId, entry.materialName, entry.entity, false};
        if (!MatchesExpectedHash(registry, entry)) { lastError_ = AssetRefreshExecutorError::PlanStale; results.push_back(std::move(receipt)); receipts_ = std::move(results); return false; }
        bool succeeded = false;
        switch (entry.action) {
            case AssetRefreshAction::RefreshTexture:
                succeeded = textures.Refresh(registry, entry.assetId);
                break;
            case AssetRefreshAction::RefreshMesh:
                succeeded = meshes.Refresh(registry, entry.assetId);
                break;
            case AssetRefreshAction::RefreshMaterial:
                succeeded = materials.Refresh(registry, entry.assetId, entry.materialName);
                break;
            case AssetRefreshAction::RebindSceneInstance: {
                const SceneMeshInstance* instance = FindInstance(scene, entry.entity);
                if (instance == nullptr) { lastError_ = AssetRefreshExecutorError::MissingInstance; results.push_back(std::move(receipt)); receipts_ = std::move(results); return false; }
                const CpuMeshResource* mesh = meshes.Find(instance->sourceAssetId);
                if (mesh == nullptr) { lastError_ = AssetRefreshExecutorError::MissingResource; results.push_back(std::move(receipt)); receipts_ = std::move(results); return false; }
                if (!meshes.IsCurrent(registry, instance->sourceAssetId)) { lastError_ = AssetRefreshExecutorError::StaleResource; results.push_back(std::move(receipt)); receipts_ = std::move(results); return false; }
                if (!instance->sourceMaterialAssetId.empty()) {
                    const CpuMaterialResource* material = materials.Find(instance->sourceMaterialAssetId, instance->sourceMaterialName);
                    if (material == nullptr) { lastError_ = AssetRefreshExecutorError::MissingResource; results.push_back(std::move(receipt)); receipts_ = std::move(results); return false; }
                    if (!materials.IsCurrent(registry, instance->sourceMaterialAssetId, instance->sourceMaterialName)) { lastError_ = AssetRefreshExecutorError::StaleResource; results.push_back(std::move(receipt)); receipts_ = std::move(results); return false; }
                    succeeded = scene.RefreshStaged(entry.entity, *mesh, *material);
                } else {
                    MeshMaterial material = instance->material;
                    if (!instance->sourceTextureAssetId.empty()) {
                        const CpuTextureResource* texture = textures.Find(instance->sourceTextureAssetId);
                        if (texture == nullptr) { lastError_ = AssetRefreshExecutorError::MissingResource; results.push_back(std::move(receipt)); receipts_ = std::move(results); return false; }
                        if (!textures.IsCurrent(registry, instance->sourceTextureAssetId)) { lastError_ = AssetRefreshExecutorError::StaleResource; results.push_back(std::move(receipt)); receipts_ = std::move(results); return false; }
                        material.texture = texture;
                    } else {
                        material.texture = nullptr;
                    }
                    succeeded = scene.RefreshStaged(entry.entity, *mesh, material);
                }
                break;
            }
        }
        receipt.succeeded = succeeded;
        results.push_back(std::move(receipt));
        if (!succeeded) { lastError_ = AssetRefreshExecutorError::ActionFailed; receipts_ = std::move(results); return false; }
    }
    receipts_ = std::move(results);
    lastError_ = AssetRefreshExecutorError::None;
    return true;
}

bool AssetRefreshExecutor::ExecuteAtomic(const std::vector<AssetRefreshPlanEntry>& plan, const AssetRegistry& registry, TextureStagingStore& textures, MeshStagingStore& meshes, MaterialStagingStore& materials, SceneMeshAdapter& scene) {
    AssetRefreshExecutor candidateExecutor = *this;
    TextureStagingStore candidateTextures = textures; MeshStagingStore candidateMeshes = meshes; MaterialStagingStore candidateMaterials = materials; SceneMeshAdapter candidateScene = scene;
    if (!candidateExecutor.Execute(plan, registry, candidateTextures, candidateMeshes, candidateMaterials, candidateScene)) { lastError_ = candidateExecutor.LastError(); return false; }
    textures = std::move(candidateTextures); meshes = std::move(candidateMeshes); materials = std::move(candidateMaterials); scene = std::move(candidateScene);
    preflightReceipts_ = std::move(candidateExecutor.preflightReceipts_); receipts_ = std::move(candidateExecutor.receipts_); lastError_ = AssetRefreshExecutorError::None;
    return true;
}

} // namespace NeoEngine
