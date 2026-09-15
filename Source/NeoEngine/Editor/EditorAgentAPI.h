#pragma once

#include "Runtime/EditorSceneSession.h"
#include "Runtime/AssetRegistry.h"
#include <string>
#include <vector>

namespace NeoEngine {

enum class EditorAgentCommandType {
    SpawnActor,
    DeleteActor,
    DuplicateActor,
    SetTransform,
    ReparentActor,
    SelectActor,
    SetActorProperties,
    SaveScene,
    Undo,
    Redo,
    QueryScene,
    Unknown
};

struct EditorAgentRequest {
    EditorAgentCommandType commandType = EditorAgentCommandType::Unknown;
    uint32_t actorId = 0;
    uint32_t newActorId = 0;
    uint32_t parentId = 0;
    EditorSceneActorKind kind = EditorSceneActorKind::Empty;
    Transform3 transform{};
    std::string name;
    std::string assetId;
    std::string materialAssetId;
    std::string textureAssetId;
    uint32_t spriteRgba = 0xFFFFFFFFU;
    std::vector<uint32_t> selectionIds;
};

struct EditorAgentResponse {
    bool success = false;
    std::string error;
    uint64_t sceneRevision = 0;
    uint32_t selectedActorId = 0;
    size_t actorCount = 0;
    std::string jsonPayload;
};

class EditorAgentAPI {
public:
    static EditorAgentResponse ProcessRequest(EditorSceneSession& session, const AssetRegistry& assets, const EditorAgentRequest& request);
    static EditorAgentResponse ProcessJsonCommand(EditorSceneSession& session, const AssetRegistry& assets, const std::string& jsonString);
    static std::string SerializeSceneToJSON(const EditorSceneSession& session);
};

} // namespace NeoEngine
