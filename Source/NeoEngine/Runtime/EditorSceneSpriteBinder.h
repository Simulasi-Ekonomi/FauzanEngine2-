#pragma once

#include "EditorSceneDocument.h"
#include "SceneSpriteAdapter.h"
#include "TextureStaging.h"

#include <cstdint>

namespace NeoEngine {

enum class EditorSceneSpriteBinderError : uint8_t { None, InvalidDocument, Capacity, MissingTextureAsset, TextureStageFailed, MissingSceneEntity, SceneSpriteRejected };

// Binds SceneDocument v3 Sprite actors only; it does not execute scenes or
// alter SceneWorld ownership, agent authority, filesystem, or network state.
class EditorSceneSpriteBinder {
public:
    static constexpr size_t kMaxBindings = SceneSpriteAdapter::kMaxInstances;
    bool BindDocumentAssets(const EditorSceneDocument& document, const EditorSceneDocumentAdapter& documentAdapter, const AssetRegistry& assets, TextureStagingStore& textures, SceneSpriteAdapter& target);
    [[nodiscard]] EditorSceneSpriteBinderError LastError() const { return lastError_; }
private:
    bool Fail(EditorSceneSpriteBinderError error);
    EditorSceneSpriteBinderError lastError_ = EditorSceneSpriteBinderError::None;
};

} // namespace NeoEngine
