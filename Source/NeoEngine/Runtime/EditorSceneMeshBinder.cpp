#include "Runtime/EditorSceneMeshBinder.h"

#include <unordered_map>
#include <unordered_set>

namespace NeoEngine {

bool EditorSceneMeshBinder::Fail(EditorSceneMeshBinderError error) {
    lastError_ = error;
    return false;
}

bool EditorSceneMeshBinder::Bind(const EditorSceneDocument& document,
                                 const EditorSceneDocumentAdapter& documentAdapter,
                                 const AssetRegistry& assets,
                                 MeshStagingStore& meshes,
                                 MaterialStagingStore& materials,
                                 const std::vector<EditorSceneMeshMaterialBinding>& bindings,
                                 SceneMeshAdapter& target) {
    if (document.version < EditorSceneDocument::kMinSupportedVersion || document.version > EditorSceneDocument::kVersion || bindings.size() > kMaxBindings) {
        return Fail(bindings.size() > kMaxBindings ? EditorSceneMeshBinderError::Capacity : EditorSceneMeshBinderError::InvalidDocument);
    }

    std::unordered_map<uint32_t, const EditorSceneActor*> meshActors;
    meshActors.reserve(document.actors.size());
    for (const EditorSceneActor& actor : document.actors) {
        if (actor.kind == EditorSceneActorKind::Mesh) {
            if (actor.assetId.empty()) return Fail(EditorSceneMeshBinderError::MissingMeshAsset);
            if (!meshActors.emplace(actor.id, &actor).second) return Fail(EditorSceneMeshBinderError::InvalidDocument);
        }
    }
    std::unordered_set<uint32_t> seenBindings;
    seenBindings.reserve(bindings.size());
    for (const EditorSceneMeshMaterialBinding& binding : bindings) {
        if (binding.actorId == 0 || !seenBindings.insert(binding.actorId).second) return Fail(EditorSceneMeshBinderError::DuplicateActorBinding);
    }
    if (meshActors.size() > kMaxBindings || meshActors.size() != bindings.size()) {
        return Fail(meshActors.size() > kMaxBindings ? EditorSceneMeshBinderError::Capacity : EditorSceneMeshBinderError::InvalidDocument);
    }

    SceneMeshAdapter candidate;
    for (const EditorSceneMeshMaterialBinding& binding : bindings) {
        const auto actor = meshActors.find(binding.actorId);
        if (actor == meshActors.end()) return Fail(EditorSceneMeshBinderError::UnknownActor);
        if (binding.materialAssetId.empty() || binding.materialName.empty()) return Fail(EditorSceneMeshBinderError::MissingMaterialReference);

        const AssetDefinition* meshAsset = assets.Find(actor->second->assetId);
        if (meshAsset == nullptr) return Fail(EditorSceneMeshBinderError::MissingMeshAsset);
        if (meshAsset->kind != AssetKind::Mesh || meshAsset->state != AssetState::Ready) return Fail(EditorSceneMeshBinderError::MeshAssetKindMismatch);
        const AssetDefinition* materialAsset = assets.Find(binding.materialAssetId);
        if (materialAsset == nullptr || materialAsset->kind != AssetKind::Material || materialAsset->state != AssetState::Ready) return Fail(EditorSceneMeshBinderError::MaterialStageFailed);

        if (meshes.Find(actor->second->assetId) == nullptr) {
            if (!meshes.StageObj(assets, actor->second->assetId)) return Fail(EditorSceneMeshBinderError::MeshStageFailed);
        } else if (!meshes.IsCurrent(assets, actor->second->assetId) && !meshes.Refresh(assets, actor->second->assetId)) {
            return Fail(EditorSceneMeshBinderError::MeshStageFailed);
        }
        if (materials.Find(binding.materialAssetId, binding.materialName) == nullptr) {
            if (!materials.StageMtl(assets, binding.materialAssetId, binding.materialName)) return Fail(EditorSceneMeshBinderError::MaterialStageFailed);
        } else if (!materials.IsCurrent(assets, binding.materialAssetId, binding.materialName) && !materials.Refresh(assets, binding.materialAssetId, binding.materialName)) {
            return Fail(EditorSceneMeshBinderError::MaterialStageFailed);
        }

        const SceneEntity* entity = documentAdapter.EntityForActor(binding.actorId);
        const CpuMeshResource* mesh = meshes.Find(actor->second->assetId);
        const CpuMaterialResource* material = materials.Find(binding.materialAssetId, binding.materialName);
        if (entity == nullptr) return Fail(EditorSceneMeshBinderError::MissingSceneEntity);
        if (mesh == nullptr || material == nullptr || !candidate.AddStaged(*entity, *mesh, *material)) return Fail(EditorSceneMeshBinderError::SceneMeshRejected);
    }

    target = std::move(candidate);
    lastError_ = EditorSceneMeshBinderError::None;
    return true;
}

bool EditorSceneMeshBinder::BindDocumentAssets(const EditorSceneDocument& document,
                                               const EditorSceneDocumentAdapter& documentAdapter,
                                               const AssetRegistry& assets,
                                               MeshStagingStore& meshes,
                                               MaterialStagingStore& materials,
                                               TextureStagingStore& textures,
                                               SceneMeshAdapter& target) {
    if (document.version != EditorSceneDocument::kVersion) return Fail(EditorSceneMeshBinderError::InvalidDocument);
    SceneMeshAdapter candidate;
    size_t meshCount = 0;
    for (const EditorSceneActor& actor : document.actors) {
        if (actor.kind != EditorSceneActorKind::Mesh) continue;
        ++meshCount;
        if (meshCount > kMaxBindings || actor.assetId.empty()) return Fail(meshCount > kMaxBindings ? EditorSceneMeshBinderError::Capacity : EditorSceneMeshBinderError::MissingMeshAsset);
        if (actor.materialAssetId.empty() || actor.materialName.empty()) return Fail(EditorSceneMeshBinderError::MissingMaterialReference);
        const AssetDefinition* meshAsset = assets.Find(actor.assetId);
        const AssetDefinition* materialAsset = assets.Find(actor.materialAssetId);
        if (meshAsset == nullptr) return Fail(EditorSceneMeshBinderError::MissingMeshAsset);
        if (meshAsset->kind != AssetKind::Mesh || meshAsset->state != AssetState::Ready) return Fail(EditorSceneMeshBinderError::MeshAssetKindMismatch);
        if (materialAsset == nullptr || materialAsset->kind != AssetKind::Material || materialAsset->state != AssetState::Ready) return Fail(EditorSceneMeshBinderError::MaterialStageFailed);
        if (meshes.Find(actor.assetId) == nullptr ? !meshes.StageObj(assets, actor.assetId) : (!meshes.IsCurrent(assets, actor.assetId) && !meshes.Refresh(assets, actor.assetId))) return Fail(EditorSceneMeshBinderError::MeshStageFailed);
        if (materials.Find(actor.materialAssetId, actor.materialName) == nullptr ? !materials.StageMtl(assets, actor.materialAssetId, actor.materialName) : (!materials.IsCurrent(assets, actor.materialAssetId, actor.materialName) && !materials.Refresh(assets, actor.materialAssetId, actor.materialName))) return Fail(EditorSceneMeshBinderError::MaterialStageFailed);
        const CpuTextureResource* texture = nullptr;
        if (!actor.textureAssetId.empty()) {
            const AssetDefinition* textureAsset = assets.Find(actor.textureAssetId);
            const std::vector<uint8_t>* textureBytes = assets.Data(actor.textureAssetId);
            if (textureAsset == nullptr || textureAsset->kind != AssetKind::Texture || textureAsset->state != AssetState::Ready || textureBytes == nullptr || textureBytes->size() < 2U) return Fail(EditorSceneMeshBinderError::TextureStageFailed);
            bool staged = true;
            if (textures.Find(actor.textureAssetId) == nullptr) {
                if ((*textureBytes)[0] == 'P' && (*textureBytes)[1] == '6') staged = textures.StagePpm(assets, actor.textureAssetId);
                else if ((*textureBytes)[0] == 'B' && (*textureBytes)[1] == 'M') staged = textures.StageBmp(assets, actor.textureAssetId);
                else staged = false;
            } else if (!textures.IsCurrent(assets, actor.textureAssetId)) staged = textures.Refresh(assets, actor.textureAssetId);
            if (!staged) return Fail(EditorSceneMeshBinderError::TextureStageFailed);
            texture = textures.Find(actor.textureAssetId);
            if (texture == nullptr) return Fail(EditorSceneMeshBinderError::TextureStageFailed);
        }
        const SceneEntity* entity = documentAdapter.EntityForActor(actor.id);
        const CpuMeshResource* mesh = meshes.Find(actor.assetId);
        const CpuMaterialResource* material = materials.Find(actor.materialAssetId, actor.materialName);
        if (entity == nullptr) return Fail(EditorSceneMeshBinderError::MissingSceneEntity);
        if (mesh == nullptr || material == nullptr || !candidate.AddStaged(*entity, *mesh, *material, texture)) return Fail(EditorSceneMeshBinderError::SceneMeshRejected);
    }
    if (meshCount == 0U) return Fail(EditorSceneMeshBinderError::InvalidDocument);
    target = std::move(candidate);
    lastError_ = EditorSceneMeshBinderError::None;
    return true;
}

} // namespace NeoEngine
