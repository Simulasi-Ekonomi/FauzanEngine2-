#pragma once

#include "EditorSceneDocument.h"
#include "MaterialStaging.h"
#include "MeshStaging.h"
#include "SceneMeshAdapter.h"
#include "TextureStaging.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {

enum class EditorSceneMeshBinderError : uint8_t {
    None,
    InvalidDocument,
    Capacity,
    DuplicateActorBinding,
    UnknownActor,
    ActorIsNotMesh,
    MissingMeshAsset,
    MeshAssetKindMismatch,
    MissingMaterialReference,
    MaterialStageFailed,
    MeshStageFailed,
    MissingSceneEntity,
    SceneMeshRejected,
    TextureStageFailed,
};

struct EditorSceneMeshMaterialBinding {
    uint32_t actorId = 0;
    std::string materialAssetId;
    std::string materialName;
};

// Binds only already-approved authoring mesh actors. It has no filesystem,
// network, agent, or runtime-authority side effects.
class EditorSceneMeshBinder {
public:
    static constexpr size_t kMaxBindings = SceneMeshAdapter::kMaxInstances;

    bool Bind(const EditorSceneDocument& document,
              const EditorSceneDocumentAdapter& documentAdapter,
              const AssetRegistry& assets,
              MeshStagingStore& meshes,
              MaterialStagingStore& materials,
              const std::vector<EditorSceneMeshMaterialBinding>& bindings,
              SceneMeshAdapter& target);
    bool BindDocumentAssets(const EditorSceneDocument& document,
                            const EditorSceneDocumentAdapter& documentAdapter,
                            const AssetRegistry& assets,
                            MeshStagingStore& meshes,
                            MaterialStagingStore& materials,
                            TextureStagingStore& textures,
                            SceneMeshAdapter& target);

    [[nodiscard]] EditorSceneMeshBinderError LastError() const { return lastError_; }

private:
    bool Fail(EditorSceneMeshBinderError error);
    EditorSceneMeshBinderError lastError_ = EditorSceneMeshBinderError::None;
};

} // namespace NeoEngine
