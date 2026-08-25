#include "Runtime/AssetRegistry.h"
#include "Runtime/FarmSpriteRenderAdapter.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/TextureStaging.h"
#include "Systems/FarmSystem.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <array>
#include <cstdio>

namespace {
std::vector<uint8_t> Ppm(uint8_t red, uint8_t green, uint8_t blue) { return {'P', '6', '\n', '1', ' ', '1', '\n', '2', '5', '5', '\n', red, green, blue}; }
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
    config.npcCount = 2;
    if (!world.Initialize(farm, trust, "sprite-farm-player", config) || !farm.Till(0, 0) || !farm.Plant(0, 0, FarmCrop::Wheat) || !farm.Water(0, 0) || !farm.Tick(3) || !world.SetGovernmentPolicy(FarmGovernmentPolicy::ConstructionPermits, true)) return 1;
    uint64_t permit = 0;
    uint32_t building = 0;
    if (!world.IssueBuildingPermit(FarmBuildingType::Barn, permit) || !world.PlaceBuilding(permit, 2, 2, building) || !world.SetCharacterState({1, 1, 3})) return 1;
    FarmSpriteAssetSet set{ids[0], ids[1], ids[2], ids[3], ids[4], ids[5], ids[6], ids[7], ids[8], ids[9], ids[10], ids[11], ids[12], ids[13], ids[14], ids[15]};
    TextureStagingStore textures;
    SoftwareRenderer renderer;
    FarmSpriteRenderAdapter adapter;
    if (!renderer.Initialize(96, 96) || !adapter.RenderWorld(farm, world, set, assets, textures, renderer) || textures.ResourceCount() != ids.size()) return 1;
    const uint64_t firstHash = renderer.FrameHash();
    if (firstHash == 0U || !adapter.RenderWorld(farm, world, set, assets, textures, renderer) || renderer.FrameHash() != firstHash) return 1;
    FarmSpriteAssetSet missing = set;
    missing.player = "";
    if (adapter.RenderWorld(farm, world, missing, assets, textures, renderer) || adapter.LastError() != FarmSpriteRenderError::InvalidAssetConfig || renderer.FrameHash() != firstHash) return 1;
    if (!assets.ReplaceBytes(set.emptyTile, {'P', '6', '\n', '1', ' ', '1', '\n', '2', '5', '5', '\n', 1}) || adapter.RenderWorld(farm, world, set, assets, textures, renderer) || adapter.LastError() != FarmSpriteRenderError::TextureStageFailed || renderer.FrameHash() != firstHash) return 1;
    std::printf("FARM_SPRITE_RENDER_SMOKE_OK textures=%zu buildings=%u npcs=%u deterministic=1 atomic=1 hash=%llu\n", textures.ResourceCount(), world.Snapshot().buildings, world.Snapshot().npcs, static_cast<unsigned long long>(firstHash));
    return 0;
}
