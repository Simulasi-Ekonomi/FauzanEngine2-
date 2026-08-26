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
    if (left.action == AssetRefreshAction::RebindSceneInstance || left.action == AssetRefreshAction::RefreshSpriteInstance) return left.entity == right.entity;
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

bool AssetRefreshExecutor::ExecuteCombinedAtomic(const std::vector<AssetRefreshPlanEntry>& plan, const AssetRegistry& registry, TextureStagingStore& textures, MeshStagingStore& meshes, MaterialStagingStore& materials, SceneMeshAdapter& scene, SceneSpriteAdapter& sprites) {
    if (plan.size() > kMaxReceipts || HasDuplicateAction(plan)) { lastError_ = plan.size() > kMaxReceipts ? AssetRefreshExecutorError::Capacity : AssetRefreshExecutorError::PlanInvalid; return false; }
    AssetRefreshExecutor candidate = *this;
    TextureStagingStore candidateTextures = textures; MeshStagingStore candidateMeshes = meshes; MaterialStagingStore candidateMaterials = materials; SceneMeshAdapter candidateScene = scene; SceneSpriteAdapter candidateSprites = sprites;
    std::vector<AssetRefreshPlanEntry> resourceEntries; std::vector<AssetRefreshPlanEntry> spriteEntries;
    resourceEntries.reserve(plan.size()); spriteEntries.reserve(plan.size());
    for (const AssetRefreshPlanEntry& entry : plan) {
        if (entry.action == AssetRefreshAction::RefreshSpriteInstance) spriteEntries.push_back(entry);
        else if (entry.action == AssetRefreshAction::RefreshTexture || entry.action == AssetRefreshAction::RefreshMesh || entry.action == AssetRefreshAction::RefreshMaterial || entry.action == AssetRefreshAction::RebindSceneInstance) resourceEntries.push_back(entry);
        else { lastError_ = AssetRefreshExecutorError::PlanInvalid; return false; }
    }
    if (!resourceEntries.empty() && !candidate.Execute(resourceEntries, registry, candidateTextures, candidateMeshes, candidateMaterials, candidateScene)) { lastError_ = candidate.LastError(); return false; }
    if (!spriteEntries.empty() && !candidate.ExecuteSpritesAtomic(spriteEntries, registry, candidateTextures, candidateSprites)) { lastError_ = candidate.LastError(); return false; }
    std::vector<AssetRefreshPreflightReceipt> combinedPreflight; std::vector<AssetRefreshReceipt> combinedReceipts;
    combinedPreflight.reserve(plan.size()); combinedReceipts.reserve(plan.size());
    for (const AssetRefreshPlanEntry& entry : plan) { combinedPreflight.push_back({entry.action, entry.assetId, entry.materialName, entry.entity, true}); combinedReceipts.push_back({entry.action, entry.assetId, entry.materialName, entry.entity, true}); }
    textures = std::move(candidateTextures); meshes = std::move(candidateMeshes); materials = std::move(candidateMaterials); scene = std::move(candidateScene); sprites = std::move(candidateSprites);
    preflightReceipts_ = std::move(combinedPreflight); receipts_ = std::move(combinedReceipts); lastError_ = AssetRefreshExecutorError::None;
    return true;
}

bool AssetRefreshExecutor::ExecutePrefabsAtomic(const std::vector<AssetRefreshPlanEntry>& plan, const AssetRegistry& registry, PrefabStagingStore& prefabs) {
    if (plan.size() > kMaxReceipts || HasDuplicateAction(plan)) { lastError_ = plan.size() > kMaxReceipts ? AssetRefreshExecutorError::Capacity : AssetRefreshExecutorError::PlanInvalid; return false; }
    AssetRefreshExecutor candidate = *this; PrefabStagingStore candidatePrefabs = prefabs;
    std::vector<AssetRefreshPreflightReceipt> preflight; preflight.reserve(plan.size());
    for (const AssetRefreshPlanEntry& entry : plan) {
        AssetRefreshPreflightReceipt receipt{entry.action, entry.assetId, entry.materialName, entry.entity, false};
        if (entry.action != AssetRefreshAction::RefreshPrefab) { lastError_ = AssetRefreshExecutorError::PlanInvalid; return false; }
        if (!MatchesExpectedHash(registry, entry)) { lastError_ = AssetRefreshExecutorError::PlanStale; return false; }
        const bool valid = candidatePrefabs.Find(entry.assetId) != nullptr && !candidatePrefabs.IsCurrent(registry, entry.assetId) && candidatePrefabs.CanRefresh(registry, entry.assetId);
        receipt.structurallyValid = valid; preflight.push_back(std::move(receipt));
        if (!valid) { lastError_ = candidatePrefabs.Find(entry.assetId) != nullptr ? AssetRefreshExecutorError::ProbeFailed : AssetRefreshExecutorError::StaleResource; return false; }
    }
    std::vector<AssetRefreshReceipt> results; results.reserve(plan.size());
    for (const AssetRefreshPlanEntry& entry : plan) {
        AssetRefreshReceipt receipt{entry.action, entry.assetId, entry.materialName, entry.entity, false};
        const bool refreshed = candidatePrefabs.Refresh(registry, entry.assetId);
        receipt.succeeded = refreshed; results.push_back(std::move(receipt));
        if (!refreshed) { lastError_ = AssetRefreshExecutorError::ActionFailed; return false; }
    }
    candidate.preflightReceipts_ = std::move(preflight); candidate.receipts_ = std::move(results);
    prefabs = std::move(candidatePrefabs); preflightReceipts_ = std::move(candidate.preflightReceipts_); receipts_ = std::move(candidate.receipts_); lastError_ = AssetRefreshExecutorError::None;
    return true;
}

bool AssetRefreshExecutor::ExecuteSpritesAtomic(const std::vector<AssetRefreshPlanEntry>& plan, const AssetRegistry& registry, TextureStagingStore& textures, SceneSpriteAdapter& sprites) {
    if (plan.size() > kMaxReceipts || HasDuplicateAction(plan)) { lastError_ = plan.size() > kMaxReceipts ? AssetRefreshExecutorError::Capacity : AssetRefreshExecutorError::PlanInvalid; return false; }
    AssetRefreshExecutor candidate = *this; TextureStagingStore candidateTextures = textures; SceneSpriteAdapter candidateSprites = sprites;
    auto fail = [this, &candidate](AssetRefreshExecutorError error) { candidate.lastError_ = error; lastError_ = error; return false; };
    std::vector<AssetRefreshPreflightReceipt> preflight; std::vector<AssetRefreshPlanEntry> prior; preflight.reserve(plan.size()); prior.reserve(plan.size());
    for (const AssetRefreshPlanEntry& entry : plan) {
        AssetRefreshPreflightReceipt receipt{entry.action, entry.assetId, entry.materialName, entry.entity, false};
        if (!MatchesExpectedHash(registry, entry)) return fail(AssetRefreshExecutorError::PlanStale);
        bool valid = false;
        if (entry.action == AssetRefreshAction::RefreshTexture) valid = candidateTextures.Find(entry.assetId) != nullptr && !candidateTextures.IsCurrent(registry, entry.assetId) && candidateTextures.CanRefresh(registry, entry.assetId);
        else if (entry.action == AssetRefreshAction::RefreshSpriteInstance) {
            std::string sourceId; uint64_t sourceHash = 0U;
            if (!candidateSprites.InspectStagedTexture(entry.entity, sourceId, sourceHash)) return fail(AssetRefreshExecutorError::MissingInstance);
            const CpuTextureResource* texture = candidateTextures.Find(entry.assetId);
            const bool textureReady = texture != nullptr && (candidateTextures.IsCurrent(registry, entry.assetId) || ContainsRefresh(prior, AssetRefreshAction::RefreshTexture, entry.assetId));
            valid = sourceId == entry.assetId && sourceHash != entry.expectedHash && textureReady;
        } else return fail(AssetRefreshExecutorError::PlanInvalid);
        receipt.structurallyValid = valid; preflight.push_back(std::move(receipt));
        if (!valid) return fail(candidateTextures.Find(entry.assetId) != nullptr ? AssetRefreshExecutorError::ProbeFailed : AssetRefreshExecutorError::StaleResource);
        prior.push_back(entry);
    }
    std::vector<AssetRefreshReceipt> results; results.reserve(plan.size());
    for (const AssetRefreshPlanEntry& entry : plan) {
        AssetRefreshReceipt receipt{entry.action, entry.assetId, entry.materialName, entry.entity, false};
        if (!MatchesExpectedHash(registry, entry)) return fail(AssetRefreshExecutorError::PlanStale);
        bool ok = false;
        if (entry.action == AssetRefreshAction::RefreshTexture) ok = candidateTextures.Find(entry.assetId) != nullptr && !candidateTextures.IsCurrent(registry, entry.assetId) && candidateTextures.Refresh(registry, entry.assetId);
        else if (entry.action == AssetRefreshAction::RefreshSpriteInstance) { std::string sourceId; uint64_t sourceHash = 0U; const CpuTextureResource* texture = candidateTextures.Find(entry.assetId); ok = candidateSprites.InspectStagedTexture(entry.entity, sourceId, sourceHash) && sourceId == entry.assetId && texture != nullptr && candidateTextures.IsCurrent(registry, entry.assetId) && sourceHash != texture->sourceHash && candidateSprites.RefreshStaged(entry.entity, *texture); }
        else return fail(AssetRefreshExecutorError::PlanInvalid);
        receipt.succeeded = ok; results.push_back(std::move(receipt));
        if (!ok) return fail(AssetRefreshExecutorError::ActionFailed);
    }
    candidate.preflightReceipts_ = std::move(preflight); candidate.receipts_ = std::move(results);
    textures = std::move(candidateTextures); sprites = std::move(candidateSprites); preflightReceipts_ = std::move(candidate.preflightReceipts_); receipts_ = std::move(candidate.receipts_); lastError_ = AssetRefreshExecutorError::None; return true;
}

} // namespace NeoEngine
