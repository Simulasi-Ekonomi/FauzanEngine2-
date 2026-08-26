#include "Runtime/AssetRegistry.h"
#include "Runtime/FarmRuntimeSession.h"
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
}

int main() {
    using namespace NeoEngine;
    const std::array<std::string, 16> ids{"tile.empty", "tile.tilled", "tile.growing", "tile.harvestable", "building.farmhouse", "building.barn", "building.silo", "building.market", "building.workshop", "building.townhall", "npc.farmer", "npc.builder", "npc.merchant", "npc.quest", "npc.ranger", "player"};
    AssetRegistry assets;
    for (size_t index = 0; index < ids.size(); ++index) if (!assets.ImportBytes(ids[index], AssetKind::Texture, {}, Ppm(static_cast<uint8_t>(20U + index * 7U), static_cast<uint8_t>(40U + index * 5U), static_cast<uint8_t>(60U + index * 3U))) || !assets.MarkReady(ids[index])) return 1;
    FarmSystem farm(4, 4, 100);
    TrustSafetySystem trust;
    FarmWorldTool world;
    FarmWorldConfig config{};
    config.worldWidth = 4;
    config.worldHeight = 4;
    config.npcCount = 1;
    if (!world.Initialize(farm, trust, "session-farm-player", config) || !world.SetCharacterState({1, 1, 2})) return 1;
    FarmSpriteAssetSet set{ids[0], ids[1], ids[2], ids[3], ids[4], ids[5], ids[6], ids[7], ids[8], ids[9], ids[10], ids[11], ids[12], ids[13], ids[14], ids[15]};
    TextureStagingStore textures;
    SoftwareRenderer renderer;
    InputState input;
    FarmRuntimeSession session;
    if (!renderer.Initialize(96, 96) || !BindInput(input) || !session.Initialize(farm, world, set, assets, textures, renderer) || session.LastFrameReceipt().frame!=0U || !input.Push(kRight, true) || !session.Frame(input) || session.FrameCount() != 1U || session.LastFrameReceipt().frame!=1U || session.LastFrameReceipt().framebufferHash!=renderer.FrameHash() || session.LastFrameReceipt().telemetry.simulationTick!=farm.SimulationTick() || session.LastFrameReceipt().input.kind!=FarmPlayerInputKind::Movement || session.LastFrameReceipt().input.x!=2U || session.LastFrameReceipt().input.z!=1U || world.Character().x != 2U || world.Character().z != 1U) return 1;
    const uint64_t afterMove = renderer.FrameHash();
    if (!input.Push(kRight, false) || !session.Frame(input) || !input.Push(kInteract, true) || !session.Frame(input) || farm.TileStateAt(2, 1) != FarmTileState::Tilled || session.FrameCount() != 3U || session.LastFrameReceipt().input.kind!=FarmPlayerInputKind::Action || session.LastFrameReceipt().input.action!=FarmPlayerAction::Till || session.LastFrameReceipt().input.x!=2U || session.LastFrameReceipt().input.z!=1U) return 1;
    const uint64_t afterTill = renderer.FrameHash();
    if (afterMove == 0U || afterTill == 0U || afterMove == afterTill) return 1;
    const uint64_t preservedHash = renderer.FrameHash();
    const uint64_t preservedFrames = session.FrameCount();
    const FarmRuntimeFrameReceipt preservedReceipt=session.LastFrameReceipt();
    if (session.Frame(input, 0) || session.LastError() != FarmRuntimeSessionError::InvalidFrameTicks || renderer.FrameHash() != preservedHash || session.FrameCount() != preservedFrames || session.LastFrameReceipt().frame!=preservedReceipt.frame || session.LastFrameReceipt().framebufferHash!=preservedReceipt.framebufferHash || session.LastFrameReceipt().telemetry.simulationTick!=preservedReceipt.telemetry.simulationTick || session.LastFrameReceipt().input.kind!=preservedReceipt.input.kind || session.LastFrameReceipt().input.action!=preservedReceipt.input.action || session.LastFrameReceipt().input.x!=preservedReceipt.input.x || session.LastFrameReceipt().input.z!=preservedReceipt.input.z || session.LastFrameReceipt().input.harvestedUnits!=preservedReceipt.input.harvestedUnits) return 1;
    InputState incomplete;
    if (session.Frame(incomplete) || session.LastError() != FarmRuntimeSessionError::InputRejected || renderer.FrameHash() != preservedHash || session.FrameCount() != preservedFrames || session.LastFrameReceipt().frame!=preservedReceipt.frame || session.LastFrameReceipt().framebufferHash!=preservedReceipt.framebufferHash || session.LastFrameReceipt().telemetry.simulationTick!=preservedReceipt.telemetry.simulationTick) return 1;
    std::printf("FARM_RUNTIME_SESSION_SMOKE_OK frames=%llu receipt=1 telemetry=1 inputWorldRender=1 framePreservation=1 hash=%llu\n", static_cast<unsigned long long>(session.FrameCount()), static_cast<unsigned long long>(preservedHash));
    return 0;
}
