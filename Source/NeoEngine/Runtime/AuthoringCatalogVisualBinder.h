#pragma once

#include "Runtime/MaterialStaging.h"
#include "Runtime/MeshStaging.h"
#include "Runtime/SceneMeshAdapter.h"
#include "Runtime/TextureStaging.h"
#include "Systems/AuthoringCatalog.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {

enum class AuthoringCatalogVisualBinderError : uint8_t { None, InvalidCatalog, Capacity, DuplicateBinding, MissingSceneEntity, MissingMeshAsset, MeshStageFailed, MissingMaterialReference, MaterialStageFailed, TextureStageFailed, SceneMeshRejected };

struct AuthoringCatalogVisualBinding {
    AuthoringSceneObjectKind kind = AuthoringSceneObjectKind::Actor;
    uint32_t definitionId = 0;
    std::string meshAssetId;
    std::string materialAssetId;
    std::string materialName;
    std::string textureAssetId;
};

class AuthoringCatalogVisualBinder {
public:
    static constexpr size_t kMaxBindings = 512;
    bool Bind(const AuthoringCatalog& catalog,
              const AssetRegistry& assets,
              MeshStagingStore& meshes,
              MaterialStagingStore& materials,
              TextureStagingStore& textures,
              const std::vector<AuthoringCatalogVisualBinding>& bindings,
              SceneMeshAdapter& target);
    [[nodiscard]] AuthoringCatalogVisualBinderError LastError() const { return lastError_; }
private:
    bool Fail(AuthoringCatalogVisualBinderError error) { lastError_ = error; return false; }
    AuthoringCatalogVisualBinderError lastError_ = AuthoringCatalogVisualBinderError::None;
};

} // namespace NeoEngine
