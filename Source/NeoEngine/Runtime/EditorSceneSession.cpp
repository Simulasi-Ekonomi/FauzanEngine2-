#include "Runtime/EditorSceneSession.h"

#include <algorithm>
#include <limits>

namespace NeoEngine {
void EditorSceneSession::PushHistory(std::vector<EditorSceneDocument>& history, const EditorSceneDocument& document) { if (history.size() == kMaxHistory) history.erase(history.begin()); history.push_back(document); }
bool EditorSceneSession::Open(const EditorSceneDocument& document, const AssetRegistry& assets) { if (!OpenCandidate(document, assets, true)) return false; undoHistory_.clear(); redoHistory_.clear(); return true; }
bool EditorSceneSession::OpenCandidate(const EditorSceneDocument& document, const AssetRegistry& assets, bool markSaved) {
    EditorSceneDocumentAdapter documentAdapter; SceneWorld world;
    if (!documentAdapter.Load(document, assets, world)) { lastError_ = EditorSceneSessionError::DocumentLoadFailed; return false; }
    MeshStagingStore meshStaging; MaterialStagingStore materialStaging; TextureStagingStore textureStaging; SceneMeshAdapter meshes; SceneSpriteAdapter sprites;
    const auto hasKind = [&document](EditorSceneActorKind kind) { return std::any_of(document.actors.begin(), document.actors.end(), [kind](const EditorSceneActor& actor) { return actor.kind == kind; }); };
    if (hasKind(EditorSceneActorKind::Mesh)) { EditorSceneMeshBinder binder; if (!binder.BindDocumentAssets(document, documentAdapter, assets, meshStaging, materialStaging, textureStaging, meshes)) { lastError_ = EditorSceneSessionError::MeshBindFailed; return false; } }
    if (hasKind(EditorSceneActorKind::Sprite)) { EditorSceneSpriteBinder binder; if (!binder.BindDocumentAssets(document, documentAdapter, assets, textureStaging, sprites)) { lastError_ = EditorSceneSessionError::SpriteBindFailed; return false; } }
    const bool keepsSelection = selectedActorId_ != 0U && std::any_of(document.actors.begin(), document.actors.end(), [this](const EditorSceneActor& actor) { return actor.id == selectedActorId_; });
    document_ = document; world_ = std::move(world); documentAdapter_ = std::move(documentAdapter); meshes_ = std::move(meshes); sprites_ = std::move(sprites); renderer_ = {}; if (!keepsSelection) selectedActorId_ = 0U;
    if (markSaved) { savedDocument_ = document_; savedRevision_ = document_.revision; }
    lastError_ = EditorSceneSessionError::None; return true;
}
bool EditorSceneSession::CommitMutation(const EditorSceneDocument& candidate, const AssetRegistry& assets) {
    const EditorSceneDocument prior = document_;
    if (!OpenCandidate(candidate, assets, false)) return false;
    PushHistory(undoHistory_, prior); redoHistory_.clear(); return true;
}
bool EditorSceneSession::OpenBytes(const std::vector<uint8_t>& bytes, const AssetRegistry& assets) { EditorSceneDocument candidate{}; EditorSceneDocumentCodec codec; if (!codec.Decode(bytes, candidate)) { lastError_ = EditorSceneSessionError::CodecDecodeFailed; return false; } return Open(candidate, assets); }
bool EditorSceneSession::UpdateTransform(uint32_t actorId, const Transform3& transform, const AssetRegistry& assets) {
    if (document_.revision == 0U || document_.revision == std::numeric_limits<uint64_t>::max()) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; }
    EditorSceneDocument candidate = document_; const auto found = std::find_if(candidate.actors.begin(), candidate.actors.end(), [actorId](const EditorSceneActor& actor) { return actor.id == actorId; });
    if (found == candidate.actors.end()) { lastError_ = EditorSceneSessionError::UnknownActor; return false; }
    found->transform = transform; ++candidate.revision; return CommitMutation(candidate, assets);
}
bool EditorSceneSession::ReparentActor(uint32_t actorId, uint32_t parentId, const AssetRegistry& assets) {
    if (document_.revision == 0U || document_.revision == std::numeric_limits<uint64_t>::max()) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; }
    EditorSceneDocument candidate = document_; const auto found = std::find_if(candidate.actors.begin(), candidate.actors.end(), [actorId](const EditorSceneActor& actor) { return actor.id == actorId; });
    if (found == candidate.actors.end()) { lastError_ = EditorSceneSessionError::UnknownActor; return false; }
    found->parentId = parentId; ++candidate.revision; return CommitMutation(candidate, assets);
}
bool EditorSceneSession::AddActor(const EditorSceneActor& actor, const AssetRegistry& assets) {
    if (document_.revision == 0U || document_.revision == std::numeric_limits<uint64_t>::max()) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; }
    EditorSceneDocument candidate = document_; candidate.actors.push_back(actor); ++candidate.revision; return CommitMutation(candidate, assets);
}
bool EditorSceneSession::DeleteActor(uint32_t actorId, const AssetRegistry& assets) {
    if (document_.revision == 0U || document_.revision == std::numeric_limits<uint64_t>::max()) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; }
    EditorSceneDocument candidate = document_; const auto found = std::find_if(candidate.actors.begin(), candidate.actors.end(), [actorId](const EditorSceneActor& actor) { return actor.id == actorId; });
    if (found == candidate.actors.end()) { lastError_ = EditorSceneSessionError::UnknownActor; return false; }
    if (std::any_of(candidate.actors.begin(), candidate.actors.end(), [actorId](const EditorSceneActor& actor) { return actor.parentId == actorId; })) { lastError_ = EditorSceneSessionError::ActorHasChildren; return false; }
    candidate.actors.erase(found); ++candidate.revision; return CommitMutation(candidate, assets);
}
bool EditorSceneSession::SelectActor(uint32_t actorId) {
    const auto found = std::find_if(document_.actors.begin(), document_.actors.end(), [actorId](const EditorSceneActor& actor) { return actor.id == actorId; });
    if (found == document_.actors.end()) { lastError_ = EditorSceneSessionError::UnknownActor; return false; }
    selectedActorId_ = actorId; lastError_ = EditorSceneSessionError::None; return true;
}
bool EditorSceneSession::Save(EditorSceneDocument& document) const { if (document_.revision == 0U) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; } document = document_; savedDocument_ = document_; savedRevision_ = document_.revision; lastError_ = EditorSceneSessionError::None; return true; }
bool EditorSceneSession::SaveBytes(std::vector<uint8_t>& bytes) const { if (document_.revision == 0U) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; } EditorSceneDocumentCodec codec; if (!codec.Encode(document_, bytes)) { lastError_ = EditorSceneSessionError::CodecEncodeFailed; return false; } savedDocument_ = document_; savedRevision_ = document_.revision; lastError_ = EditorSceneSessionError::None; return true; }
bool EditorSceneSession::RevertToSaved(const AssetRegistry& assets) { if (document_.revision == 0U || savedRevision_ == 0U) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; } if (!OpenCandidate(savedDocument_, assets, true)) return false; undoHistory_.clear(); redoHistory_.clear(); return true; }
bool EditorSceneSession::Undo(const AssetRegistry& assets) { if (undoHistory_.empty()) { lastError_ = EditorSceneSessionError::HistoryUnavailable; return false; } const EditorSceneDocument prior = document_; const EditorSceneDocument target = undoHistory_.back(); if (!OpenCandidate(target, assets, false)) return false; undoHistory_.pop_back(); PushHistory(redoHistory_, prior); return true; }
bool EditorSceneSession::Redo(const AssetRegistry& assets) { if (redoHistory_.empty()) { lastError_ = EditorSceneSessionError::HistoryUnavailable; return false; } const EditorSceneDocument prior = document_; const EditorSceneDocument target = redoHistory_.back(); if (!OpenCandidate(target, assets, false)) return false; redoHistory_.pop_back(); PushHistory(undoHistory_, prior); return true; }
std::vector<EditorSceneActor> EditorSceneSession::HierarchySnapshot() const { std::vector<EditorSceneActor> snapshot = document_.actors; std::sort(snapshot.begin(), snapshot.end(), [](const EditorSceneActor& left, const EditorSceneActor& right) { return left.id < right.id; }); return snapshot; }
bool EditorSceneSession::InspectActor(uint32_t actorId, EditorSceneActor& actor) const { const auto found = std::find_if(document_.actors.begin(), document_.actors.end(), [actorId](const EditorSceneActor& candidate) { return candidate.id == actorId; }); if (found == document_.actors.end()) { lastError_ = EditorSceneSessionError::UnknownActor; return false; } actor = *found; lastError_ = EditorSceneSessionError::None; return true; }
bool EditorSceneSession::RenderViewport(RenderCamera& camera, SoftwareRenderer& renderer, const DirectionalLight& light) { if (document_.revision == 0U) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; } if (!renderer_.Draw(world_, meshes_, sprites_, camera, renderer, light)) { lastError_ = EditorSceneSessionError::ViewportRenderFailed; return false; } lastError_ = EditorSceneSessionError::None; return true; }
} // namespace NeoEngine
