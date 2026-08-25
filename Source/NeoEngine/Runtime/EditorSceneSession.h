#pragma once

#include "EditorSceneDocumentCodec.h"
#include "EditorSceneMeshBinder.h"
#include "EditorSceneSpriteBinder.h"
#include "SceneRenderAdapter.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {
enum class EditorSceneSessionError : uint8_t { None, InvalidDocument, DocumentLoadFailed, MeshBindFailed, SpriteBindFailed, CodecDecodeFailed, CodecEncodeFailed, UnknownActor, ActorHasChildren, ViewportRenderFailed };

// Bounded in-engine editor foundation. It owns a loaded SceneDocument snapshot
// and its canonical runtime adapters; it has no desktop UI, filesystem, network,
// agent, or gameplay authority.
class EditorSceneSession {
public:
    bool Open(const EditorSceneDocument& document, const AssetRegistry& assets);
    bool OpenBytes(const std::vector<uint8_t>& bytes, const AssetRegistry& assets);
    bool UpdateTransform(uint32_t actorId, const Transform3& transform, const AssetRegistry& assets);
    bool ReparentActor(uint32_t actorId, uint32_t parentId, const AssetRegistry& assets);
    bool AddActor(const EditorSceneActor& actor, const AssetRegistry& assets);
    bool DeleteActor(uint32_t actorId, const AssetRegistry& assets);
    bool Save(EditorSceneDocument& document) const;
    bool SaveBytes(std::vector<uint8_t>& bytes) const;
    bool RevertToSaved(const AssetRegistry& assets);
    [[nodiscard]] bool HasUnsavedChanges() const { return document_.revision != 0U && document_.revision != savedRevision_; }
    [[nodiscard]] std::vector<EditorSceneActor> HierarchySnapshot() const;
    bool InspectActor(uint32_t actorId, EditorSceneActor& actor) const;
    bool RenderViewport(RenderCamera& camera, SoftwareRenderer& renderer, const DirectionalLight& light);
    [[nodiscard]] const SceneWorld& World() const { return world_; }
    [[nodiscard]] EditorSceneSessionError LastError() const { return lastError_; }
private:
    bool OpenCandidate(const EditorSceneDocument& document, const AssetRegistry& assets, bool markSaved);
    EditorSceneDocument document_{};
    mutable EditorSceneDocument savedDocument_{};
    mutable uint64_t savedRevision_ = 0;
    SceneWorld world_{};
    EditorSceneDocumentAdapter documentAdapter_{};
    SceneMeshAdapter meshes_{};
    SceneSpriteAdapter sprites_{};
    SceneRenderAdapter renderer_{};
    mutable EditorSceneSessionError lastError_ = EditorSceneSessionError::InvalidDocument;
};
} // namespace NeoEngine
