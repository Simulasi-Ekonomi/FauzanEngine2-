#include "Runtime/EditorSceneSession.h"

#include <algorithm>

namespace NeoEngine {
bool EditorSceneSession::Open(const EditorSceneDocument& document, const AssetRegistry& assets) {
    EditorSceneDocumentAdapter documentAdapter; SceneWorld world;
    if (!documentAdapter.Load(document, assets, world)) { lastError_ = EditorSceneSessionError::DocumentLoadFailed; return false; }
    MeshStagingStore meshStaging; MaterialStagingStore materialStaging; TextureStagingStore textureStaging; SceneMeshAdapter meshes; SceneSpriteAdapter sprites;
    const auto hasKind = [&document](EditorSceneActorKind kind) { return std::any_of(document.actors.begin(), document.actors.end(), [kind](const EditorSceneActor& actor) { return actor.kind == kind; }); };
    if (hasKind(EditorSceneActorKind::Mesh)) { EditorSceneMeshBinder binder; if (!binder.BindDocumentAssets(document, documentAdapter, assets, meshStaging, materialStaging, textureStaging, meshes)) { lastError_ = EditorSceneSessionError::MeshBindFailed; return false; } }
    if (hasKind(EditorSceneActorKind::Sprite)) { EditorSceneSpriteBinder binder; if (!binder.BindDocumentAssets(document, documentAdapter, assets, textureStaging, sprites)) { lastError_ = EditorSceneSessionError::SpriteBindFailed; return false; } }
    document_ = document; world_ = std::move(world); documentAdapter_ = std::move(documentAdapter); meshes_ = std::move(meshes); sprites_ = std::move(sprites); renderer_ = {}; lastError_ = EditorSceneSessionError::None; return true;
}
bool EditorSceneSession::Save(EditorSceneDocument& document) const { if (document_.revision == 0U) return false; document = document_; return true; }
std::vector<EditorSceneActor> EditorSceneSession::HierarchySnapshot() const { std::vector<EditorSceneActor> snapshot = document_.actors; std::sort(snapshot.begin(), snapshot.end(), [](const EditorSceneActor& left, const EditorSceneActor& right) { return left.id < right.id; }); return snapshot; }
bool EditorSceneSession::InspectActor(uint32_t actorId, EditorSceneActor& actor) const { const auto found = std::find_if(document_.actors.begin(), document_.actors.end(), [actorId](const EditorSceneActor& candidate) { return candidate.id == actorId; }); if (found == document_.actors.end()) { lastError_ = EditorSceneSessionError::UnknownActor; return false; } actor = *found; lastError_ = EditorSceneSessionError::None; return true; }
bool EditorSceneSession::RenderViewport(RenderCamera& camera, SoftwareRenderer& renderer, const DirectionalLight& light) { if (document_.revision == 0U) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; } if (!renderer_.Draw(world_, meshes_, sprites_, camera, renderer, light)) { lastError_ = EditorSceneSessionError::ViewportRenderFailed; return false; } lastError_ = EditorSceneSessionError::None; return true; }
} // namespace NeoEngine
