#include "Runtime/AuthoringCatalogVisualBinder.h"

#include <unordered_set>

namespace NeoEngine {
namespace {
uint64_t Key(AuthoringSceneObjectKind kind, uint32_t definitionId) { return (static_cast<uint64_t>(static_cast<uint8_t>(kind)) << 32U) | definitionId; }

bool StageTexture(const AssetRegistry& assets, TextureStagingStore& textures, const std::string& assetId, const CpuTextureResource*& texture) {
    texture = nullptr;
    if (assetId.empty()) return true;
    const AssetDefinition* definition = assets.Find(assetId);
    const std::vector<uint8_t>* bytes = assets.Data(assetId);
    if (definition == nullptr || definition->kind != AssetKind::Texture || definition->state != AssetState::Ready || bytes == nullptr || bytes->size() < 2U) return false;
    bool staged = true;
    if (textures.Find(assetId) == nullptr) {
        if ((*bytes)[0] == 'P' && (*bytes)[1] == '6') staged = textures.StagePpm(assets, assetId);
        else if ((*bytes)[0] == 'B' && (*bytes)[1] == 'M') staged = textures.StageBmp(assets, assetId);
        else staged = false;
    } else if (!textures.IsCurrent(assets, assetId)) staged = textures.Refresh(assets, assetId);
    texture = staged ? textures.Find(assetId) : nullptr;
    return texture != nullptr;
}
} // namespace

bool AuthoringCatalogVisualBinder::Bind(const AuthoringCatalog& catalog,
                                        const AssetRegistry& assets,
                                        MeshStagingStore& meshes,
                                        MaterialStagingStore& materials,
                                        TextureStagingStore& textures,
                                        const std::vector<AuthoringCatalogVisualBinding>& bindings,
                                        SceneMeshAdapter& target) {
    if (!catalog.IsSceneBound() || bindings.empty()) return Fail(AuthoringCatalogVisualBinderError::InvalidCatalog);
    if (bindings.size() > kMaxBindings) return Fail(AuthoringCatalogVisualBinderError::Capacity);
    std::unordered_set<uint64_t> seen;
    SceneMeshAdapter candidate;
    for (const AuthoringCatalogVisualBinding& binding : bindings) {
        if (binding.definitionId == 0 || binding.meshAssetId.empty() || binding.materialAssetId.empty() || binding.materialName.empty() || !seen.insert(Key(binding.kind, binding.definitionId)).second) return Fail(binding.definitionId == 0 || binding.meshAssetId.empty() || binding.materialAssetId.empty() || binding.materialName.empty() ? AuthoringCatalogVisualBinderError::MissingMaterialReference : AuthoringCatalogVisualBinderError::DuplicateBinding);
        const SceneEntity* entity = catalog.BoundEntity(binding.kind, binding.definitionId);
        if (entity == nullptr) return Fail(AuthoringCatalogVisualBinderError::MissingSceneEntity);
        const AssetDefinition* meshAsset = assets.Find(binding.meshAssetId);
        if (meshAsset == nullptr || meshAsset->kind != AssetKind::Mesh || meshAsset->state != AssetState::Ready) return Fail(AuthoringCatalogVisualBinderError::MissingMeshAsset);
        if (meshes.Find(binding.meshAssetId) == nullptr ? !meshes.StageObj(assets, binding.meshAssetId) : (!meshes.IsCurrent(assets, binding.meshAssetId) && !meshes.Refresh(assets, binding.meshAssetId))) return Fail(AuthoringCatalogVisualBinderError::MeshStageFailed);
        const AssetDefinition* materialAsset = assets.Find(binding.materialAssetId);
        if (materialAsset == nullptr || materialAsset->kind != AssetKind::Material || materialAsset->state != AssetState::Ready) return Fail(AuthoringCatalogVisualBinderError::MaterialStageFailed);
        if (materials.Find(binding.materialAssetId, binding.materialName) == nullptr ? !materials.StageMtl(assets, binding.materialAssetId, binding.materialName) : (!materials.IsCurrent(assets, binding.materialAssetId, binding.materialName) && !materials.Refresh(assets, binding.materialAssetId, binding.materialName))) return Fail(AuthoringCatalogVisualBinderError::MaterialStageFailed);
        const CpuTextureResource* texture = nullptr;
        if (!StageTexture(assets, textures, binding.textureAssetId, texture)) return Fail(AuthoringCatalogVisualBinderError::TextureStageFailed);
        const CpuMeshResource* mesh = meshes.Find(binding.meshAssetId);
        const CpuMaterialResource* material = materials.Find(binding.materialAssetId, binding.materialName);
        if (mesh == nullptr || material == nullptr || !candidate.AddStaged(*entity, *mesh, *material, texture)) return Fail(AuthoringCatalogVisualBinderError::SceneMeshRejected);
    }
    target = std::move(candidate);
    lastError_ = AuthoringCatalogVisualBinderError::None;
    return true;
}

} // namespace NeoEngine
