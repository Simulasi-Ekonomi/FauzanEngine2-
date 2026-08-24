#pragma once

#include "Runtime/EditorSceneDocument.h"

#include <cstdint>
#include <span>
#include <string>

namespace NeoEngine {

enum class LocalAuthoringBridgeError : uint8_t { None, ApprovalRequired, CorruptPayload, UnsupportedVersion, TrailingBytes, SceneRejected };

struct LocalAuthoringBridgeReceipt {
    std::string sceneId;
    uint64_t revision = 0;
    uint16_t actorCount = 0;
    uint64_t payloadDigest = 0;
};

class LocalAuthoringBridge {
public:
    bool Load(std::span<const uint8_t> payload, bool approved, const AssetRegistry& assets, SceneWorld& target, LocalAuthoringBridgeReceipt& receipt);
    [[nodiscard]] LocalAuthoringBridgeError LastError() const { return lastError_; }
    [[nodiscard]] EditorSceneDocumentError LastSceneError() const { return adapter_.LastError(); }
    [[nodiscard]] const SceneEntity* EntityForActor(uint32_t actorId) const { return adapter_.EntityForActor(actorId); }
private:
    bool Fail(LocalAuthoringBridgeError error);
    EditorSceneDocumentAdapter adapter_;
    LocalAuthoringBridgeError lastError_ = LocalAuthoringBridgeError::None;
};

} // namespace NeoEngine
