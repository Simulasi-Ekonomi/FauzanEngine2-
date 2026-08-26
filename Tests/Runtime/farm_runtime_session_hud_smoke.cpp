#include "Runtime/AssetRegistry.h"
#include "Runtime/FarmRuntimeHud.h"
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
    FarmSystem farm(4U, 4U, 100); TrustSafetySystem trust; FarmWorldTool world; FarmWorldConfig config{}; config.worldWidth = 4U; config.worldHeight = 4U; config.npcCount = 1U;
    if (!world.Initialize(farm, trust, "hud-farm-player", config) || !world.SetCharacterState({1U, 1U, 2U})) return 1;
    FarmSpriteAssetSet set{ids[0], ids[1], ids[2], ids[3], ids[4], ids[5], ids[6], ids[7], ids[8], ids[9], ids[10], ids[11], ids[12], ids[13], ids[14], ids[15]};
    TextureStagingStore textures; SoftwareRenderer renderer; InputState input; FarmRuntimeSession session; FarmRuntimeHud hud;
    if (!renderer.Initialize(96U, 96U) || !BindInput(input) || !session.Initialize(farm, world, set, assets, textures, renderer)) return 1;
    FarmRuntimeHudReceipt preserved{99U, 88U, 77U, {66U, 55U, 44U, 33, 2U, 1U, 0U, 0U, 0U, false, FarmError::None}}; const uint64_t blankHash = renderer.FrameHash();
    if (session.DrawHud(hud, preserved) || session.LastError() != FarmRuntimeSessionError::HudRejected || preserved.frame != 99U || preserved.worldFramebufferHash != 88U || preserved.hudFramebufferHash != 77U || renderer.FrameHash() != blankHash || session.FrameCount() != 0U) return 1;
    if (!session.Frame(input) || session.FrameCount() != 1U) return 1;
    const FarmRuntimeFrameReceipt worldReceipt = session.LastFrameReceipt(); const uint64_t worldHash = renderer.FrameHash(); FarmRuntimeHudReceipt overlay{};
    if (!session.DrawHud(hud, overlay) || session.LastError() != FarmRuntimeSessionError::None || overlay.frame != worldReceipt.frame || overlay.worldFramebufferHash != worldReceipt.framebufferHash || overlay.hudFramebufferHash != renderer.FrameHash() || overlay.hudFramebufferHash == worldHash || overlay.telemetry.simulationTick != worldReceipt.telemetry.simulationTick || session.FrameCount() != 1U || session.LastFrameReceipt().frame != worldReceipt.frame || session.LastFrameReceipt().framebufferHash != worldReceipt.framebufferHash) return 1;
    std::printf("FARM_RUNTIME_SESSION_HUD_SMOKE_OK frame=%llu world=%llu hud=%llu readonly=1\n", static_cast<unsigned long long>(overlay.frame), static_cast<unsigned long long>(overlay.worldFramebufferHash), static_cast<unsigned long long>(overlay.hudFramebufferHash));
    return 0;
}
