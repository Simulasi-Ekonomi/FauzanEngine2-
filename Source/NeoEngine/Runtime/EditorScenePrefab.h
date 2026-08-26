#pragma once

#include "Runtime/EditorSceneDocument.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {

enum class EditorScenePrefabError : uint8_t { None, InvalidDocument, SourceNotFound, InvalidSubtree, Capacity, InvalidMapping, ParentNotFound };

// A copyable, in-memory actor subtree captured from a validated scene document.
// It has no asset lifetime, nested-prefab, filesystem, or runtime-instancing role.
struct EditorScenePrefab {
    uint32_t rootSourceId = 0U;
    std::vector<EditorSceneActor> actors;
};

class EditorScenePrefabAdapter {
public:
    static constexpr size_t kMaxActors = 64U;

    bool Capture(const EditorSceneDocument& document, uint32_t rootActorId, EditorScenePrefab& target);
    bool AppendInstance(const EditorSceneDocument& document, const EditorScenePrefab& prefab, uint32_t parentActorId, const std::vector<uint32_t>& instanceActorIds, EditorSceneDocument& target);
    [[nodiscard]] EditorScenePrefabError LastError() const { return lastError_; }

private:
    bool Fail(EditorScenePrefabError error);
    EditorScenePrefabError lastError_ = EditorScenePrefabError::None;
};

} // namespace NeoEngine
