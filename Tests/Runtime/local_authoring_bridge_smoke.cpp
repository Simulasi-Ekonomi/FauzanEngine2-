#include "Runtime/LocalAuthoringBridge.h"

#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {
void U8(std::vector<uint8_t>& bytes, uint8_t value) { bytes.push_back(value); }
void U16(std::vector<uint8_t>& bytes, uint16_t value) { U8(bytes, static_cast<uint8_t>(value)); U8(bytes, static_cast<uint8_t>(value >> 8U)); }
void U32(std::vector<uint8_t>& bytes, uint32_t value) { U16(bytes, static_cast<uint16_t>(value)); U16(bytes, static_cast<uint16_t>(value >> 16U)); }
void U64(std::vector<uint8_t>& bytes, uint64_t value) { U32(bytes, static_cast<uint32_t>(value)); U32(bytes, static_cast<uint32_t>(value >> 32U)); }
void Float(std::vector<uint8_t>& bytes, float value) { U32(bytes, std::bit_cast<uint32_t>(value)); }
void String(std::vector<uint8_t>& bytes, const std::string& value) { U8(bytes, static_cast<uint8_t>(value.size())); bytes.insert(bytes.end(), value.begin(), value.end()); }
void Actor(std::vector<uint8_t>& bytes, uint32_t id, uint32_t parent, uint8_t kind, float x, const std::string& asset) { U32(bytes, id); U32(bytes, parent); U8(bytes, kind); Float(bytes, x); Float(bytes, 0); Float(bytes, 0); Float(bytes, 0); Float(bytes, 0); Float(bytes, 0); Float(bytes, 1); Float(bytes, 1); Float(bytes, 1); String(bytes, asset); }
std::vector<uint8_t> Payload(const std::string& asset) { std::vector<uint8_t> bytes{'N','A','B','1',1}; String(bytes, "farm-slice"); U64(bytes, 1); U16(bytes, 2); Actor(bytes, 10, 0, 1, 4, asset); Actor(bytes, 20, 10, 5, 2, ""); return bytes; }
}

int main() {
    using namespace NeoEngine;
    AssetRegistry assets;
    if (!assets.ImportBytes("mesh.cube", AssetKind::Mesh, {}, {1, 2, 3}) || !assets.MarkReady("mesh.cube")) return 1;
    SceneWorld target;
    LocalAuthoringBridge bridge;
    LocalAuthoringBridgeReceipt receipt{};
    const auto valid = Payload("mesh.cube");
    if (bridge.Load(valid, false, assets, target, receipt) || bridge.LastError() != LocalAuthoringBridgeError::ApprovalRequired || target.AliveCount() != 0) return 1;
    if (!bridge.Load(valid, true, assets, target, receipt) || receipt.sceneId != "farm-slice" || receipt.revision != 1 || receipt.actorCount != 2 || receipt.payloadDigest != 5832217868230341815ULL || target.AliveCount() != 2) return 1;
    const SceneEntity* child = bridge.EntityForActor(20);
    if (child == nullptr || target.GetTransform(*child) == nullptr || std::fabs(target.GetTransform(*child)->x - 6.0F) > 0.0001F) return 1;
    const uint32_t preserved = target.AliveCount();
    if (bridge.Load(Payload("mesh.missing"), true, assets, target, receipt) || bridge.LastError() != LocalAuthoringBridgeError::SceneRejected || bridge.LastSceneError() != EditorSceneDocumentError::MissingAsset || target.AliveCount() != preserved) return 1;
    auto trailing = valid; trailing.push_back(0);
    if (bridge.Load(trailing, true, assets, target, receipt) || bridge.LastError() != LocalAuthoringBridgeError::TrailingBytes || target.AliveCount() != preserved) return 1;
    std::printf("LOCAL_AUTHORING_BRIDGE_SMOKE_OK actors=%u approval=1 assets=1 atomic=1 digest=%llu\n", target.AliveCount(), static_cast<unsigned long long>(receipt.payloadDigest));
    return 0;
}
