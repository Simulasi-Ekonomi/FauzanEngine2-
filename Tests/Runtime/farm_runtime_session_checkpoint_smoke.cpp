#include "Runtime/AssetRegistry.h"
#include "Runtime/FarmRuntimeSession.h"
#include "Runtime/RuntimePersistence.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/TextureStaging.h"
#include "Systems/FarmSystem.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <array>
#include <cstdio>

namespace {
constexpr int32_t kUp = 1, kDown = 2, kLeft = 3, kRight = 4, kInteract = 5;
std::vector<uint8_t> Ppm(uint8_t red, uint8_t green, uint8_t blue) { return {'P', '6', '\n', '1', ' ', '1', '\n', '2', '5', '5', '\n', red, green, blue}; }
bool BindInput(NeoEngine::InputState& input) { return input.Bind("farm_move_up", kUp) && input.Bind("farm_move_down", kDown) && input.Bind("farm_move_left", kLeft) && input.Bind("farm_move_right", kRight) && input.Bind("farm_interact", kInteract); }
bool SameReceipt(const NeoEngine::FarmRuntimeFrameReceipt& left, const NeoEngine::FarmRuntimeFrameReceipt& right) {
    return left.frame == right.frame && left.framebufferHash == right.framebufferHash && left.telemetry.simulationTick == right.telemetry.simulationTick && left.telemetry.stateRevision == right.telemetry.stateRevision;
}
}

int main() {
    using namespace NeoEngine;
    const std::array<std::string, 16> ids{"tile.empty", "tile.tilled", "tile.growing", "tile.harvestable", "building.farmhouse", "building.barn", "building.silo", "building.market", "building.workshop", "building.townhall", "npc.farmer", "npc.builder", "npc.merchant", "npc.quest", "npc.ranger", "player"};
    AssetRegistry assets;
    for (size_t index = 0; index < ids.size(); ++index) if (!assets.ImportBytes(ids[index], AssetKind::Texture, {}, Ppm(static_cast<uint8_t>(20U + index * 7U), static_cast<uint8_t>(40U + index * 5U), static_cast<uint8_t>(60U + index * 3U))) || !assets.MarkReady(ids[index])) return 1;
    FarmSystem farm(4U, 4U, 100); TrustSafetySystem trust; FarmWorldTool world; FarmWorldConfig config{}; config.worldWidth = 4U; config.worldHeight = 4U; config.npcCount = 1U;
    if (!world.Initialize(farm, trust, "checkpoint-farm-player", config) || !world.SetCharacterState({1U, 1U, 2U})) return 1;
    FarmSpriteAssetSet set{ids[0], ids[1], ids[2], ids[3], ids[4], ids[5], ids[6], ids[7], ids[8], ids[9], ids[10], ids[11], ids[12], ids[13], ids[14], ids[15]};
    TextureStagingStore textures; SoftwareRenderer renderer; InputState input; FarmRuntimeSession session;
    if (!renderer.Initialize(96U, 96U) || !BindInput(input) || !session.Initialize(farm, world, set, assets, textures, renderer) || !session.Frame(input) || session.FrameCount() != 1U) return 1;
    if (!farm.Till(0U, 0U)) return 1;
    std::vector<uint8_t> checkpoint;
    if (!session.SaveCheckpoint(7U, checkpoint) || checkpoint.empty() || session.LastError() != FarmRuntimeSessionError::None) return 1;
    const std::vector<uint8_t> checkpointState = farm.Serialize(); const uint64_t checkpointFrames = session.FrameCount(); const FarmRuntimeFrameReceipt checkpointReceipt = session.LastFrameReceipt(); const uint64_t checkpointHash = renderer.FrameHash();
    if (!farm.Till(1U, 0U) || farm.TileStateAt(1U, 0U) != FarmTileState::Tilled) return 1;
    uint64_t revision = 1U;
    if (!session.RestoreCheckpoint(checkpoint, revision) || revision != 7U || farm.Serialize() != checkpointState || farm.TileStateAt(1U, 0U) != FarmTileState::Empty || session.FrameCount() != checkpointFrames || !SameReceipt(session.LastFrameReceipt(), checkpointReceipt) || renderer.FrameHash() != checkpointHash) return 1;
    const std::vector<uint8_t> preservedState = farm.Serialize(); const uint64_t preservedRevision = revision;
    RuntimeSaveEnvelope wrong{"other-world", 8U, farm.Serialize()}; RuntimePersistenceError persistenceError = RuntimePersistenceError::None; std::vector<uint8_t> wrongBytes;
    if (!RuntimeSaveCodec::Serialize(wrong, wrongBytes, persistenceError) || session.RestoreCheckpoint(wrongBytes, revision) || session.LastError() != FarmRuntimeSessionError::CheckpointDecodeFailed || farm.Serialize() != preservedState || revision != preservedRevision || session.FrameCount() != checkpointFrames || !SameReceipt(session.LastFrameReceipt(), checkpointReceipt) || renderer.FrameHash() != checkpointHash) return 1;
    std::vector<uint8_t> malformed = checkpoint; malformed.back() ^= 0x01U;
    if (session.RestoreCheckpoint(malformed, revision) || session.LastError() != FarmRuntimeSessionError::CheckpointDecodeFailed || farm.Serialize() != preservedState || revision != preservedRevision || session.FrameCount() != checkpointFrames || !SameReceipt(session.LastFrameReceipt(), checkpointReceipt) || renderer.FrameHash() != checkpointHash) return 1;
    std::vector<uint8_t> preservedBytes = checkpoint;
    if (session.SaveCheckpoint(0U, preservedBytes) || session.LastError() != FarmRuntimeSessionError::CheckpointEncodeFailed || preservedBytes != checkpoint || session.FrameCount() != checkpointFrames || !SameReceipt(session.LastFrameReceipt(), checkpointReceipt)) return 1;
    std::printf("FARM_RUNTIME_SESSION_CHECKPOINT_SMOKE_OK revision=%llu frames=%llu atomic=1 checkpoint=1\n", static_cast<unsigned long long>(revision), static_cast<unsigned long long>(session.FrameCount()));
    return 0;
}
