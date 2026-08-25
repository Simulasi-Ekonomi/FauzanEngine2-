#include "Runtime/FarmSpriteRenderAdapter.h"

#include "Runtime/AssetRegistry.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SpriteBatch.h"
#include "Runtime/TextureStaging.h"
#include "Systems/FarmSystem.h"
#include "Systems/FarmWorldTool.h"

#include <array>
#include <string_view>

namespace NeoEngine {
namespace {
constexpr uint32_t kBackground = 0xFF17324DU;

const std::string& TileAsset(const FarmSpriteAssetSet& set, FarmTileState state) { switch (state) { case FarmTileState::Empty: return set.emptyTile; case FarmTileState::Tilled: return set.tilledTile; case FarmTileState::Growing: return set.growingTile; case FarmTileState::Harvestable: return set.harvestableTile; } return set.emptyTile; }
const std::string& BuildingAsset(const FarmSpriteAssetSet& set, FarmBuildingType type) { switch (type) { case FarmBuildingType::Farmhouse: return set.farmhouse; case FarmBuildingType::Barn: return set.barn; case FarmBuildingType::Silo: return set.silo; case FarmBuildingType::Market: return set.market; case FarmBuildingType::Workshop: return set.workshop; case FarmBuildingType::TownHall: return set.townHall; } return set.farmhouse; }
const std::string& NpcAsset(const FarmSpriteAssetSet& set, FarmNpcRole role) { switch (role) { case FarmNpcRole::Farmer: return set.farmer; case FarmNpcRole::Builder: return set.builder; case FarmNpcRole::Merchant: return set.merchant; case FarmNpcRole::QuestGiver: return set.questGiver; case FarmNpcRole::Ranger: return set.ranger; } return set.farmer; }
bool QueueSprite(SpriteBatch& batch, const CpuTextureResource& texture, uint16_t x, uint16_t z, uint16_t width, uint16_t height, float scale, int16_t layer, int16_t order) { if (width == 0U || height == 0U || x >= width || z >= height) return false; const float worldX = static_cast<float>(x) + 0.5F - static_cast<float>(width) * 0.5F; const float worldY = static_cast<float>(z) + 0.5F - static_cast<float>(height) * 0.5F; return batch.Queue({worldX, worldY, 1.0F, 2.0F * scale, 2.0F * scale, layer, order, 0xFFFFFFFFU, &texture}); }
bool StageTexture(const AssetRegistry& assets, TextureStagingStore& textures, std::string_view assetId) { const AssetDefinition* definition = assets.Find(assetId); const std::vector<uint8_t>* bytes = assets.Data(assetId); if (assetId.empty() || definition == nullptr || definition->kind != AssetKind::Texture || definition->state != AssetState::Ready || bytes == nullptr || bytes->size() < 2U) return false; if (textures.Find(assetId) != nullptr) return textures.IsCurrent(assets, assetId) || textures.Refresh(assets, assetId); if ((*bytes)[0] == 'P' && (*bytes)[1] == '6') return textures.StagePpm(assets, assetId); if ((*bytes)[0] == 'B' && (*bytes)[1] == 'M') return textures.StageBmp(assets, assetId); return false; }
}

bool FarmSpriteRenderAdapter::Fail(FarmSpriteRenderError error) { lastError_ = error; return false; }

bool FarmSpriteRenderAdapter::RenderWorld(const FarmSystem& farm, const FarmWorldTool& world, const FarmSpriteAssetSet& set, const AssetRegistry& assets, TextureStagingStore& textures, SoftwareRenderer& renderer) {
    const FarmWorldSnapshot snapshot = world.Snapshot();
    if (!farm.IsReady() || !world.IsReady()) return Fail(FarmSpriteRenderError::NotReady);
    if (snapshot.worldWidth != farm.Width() || snapshot.worldHeight != farm.Height()) return Fail(FarmSpriteRenderError::WorldMismatch);
    const size_t drawCount = static_cast<size_t>(farm.Width()) * farm.Height() + world.Buildings().size() + world.Npcs().size() + 1U;
    if (drawCount > SpriteBatch::kMaxSprites) return Fail(FarmSpriteRenderError::Capacity);
    if (renderer.Width() == 0U || renderer.Height() == 0U) return Fail(FarmSpriteRenderError::RendererUnavailable);
    const std::array<std::string_view, 16> required{set.emptyTile, set.tilledTile, set.growingTile, set.harvestableTile, set.farmhouse, set.barn, set.silo, set.market, set.workshop, set.townHall, set.farmer, set.builder, set.merchant, set.questGiver, set.ranger, set.player};
    for (std::string_view id : required) if (id.empty()) return Fail(FarmSpriteRenderError::InvalidAssetConfig);
    TextureStagingStore candidateTextures = textures;
    for (std::string_view id : required) if (!StageTexture(assets, candidateTextures, id)) return Fail(FarmSpriteRenderError::TextureStageFailed);
    textures = std::move(candidateTextures);
    SpriteBatch batch;
    const auto textureFor = [&textures](const std::string& id) { return textures.Find(id); };
    for (uint16_t z = 0; z < farm.Height(); ++z) for (uint16_t x = 0; x < farm.Width(); ++x) { const CpuTextureResource* texture = textureFor(TileAsset(set, farm.TileStateAt(x, z))); if (texture == nullptr || !QueueSprite(batch, *texture, x, z, farm.Width(), farm.Height(), 0.5F, 0, static_cast<int16_t>(z * farm.Width() + x))) return Fail(FarmSpriteRenderError::QueueRejected); }
    for (const FarmWorldBuilding& building : world.Buildings()) { const CpuTextureResource* texture = textureFor(BuildingAsset(set, building.type)); if (texture == nullptr || !QueueSprite(batch, *texture, building.x, building.z, farm.Width(), farm.Height(), 0.42F, 10, static_cast<int16_t>(building.id % 32767U))) return Fail(FarmSpriteRenderError::QueueRejected); }
    for (const FarmWorldNpc& npc : world.Npcs()) { const CpuTextureResource* texture = textureFor(NpcAsset(set, npc.role)); if (texture == nullptr || !QueueSprite(batch, *texture, npc.x, npc.z, farm.Width(), farm.Height(), 0.22F, 20, static_cast<int16_t>(npc.id % 32767U))) return Fail(FarmSpriteRenderError::QueueRejected); }
    const FarmCharacterState& player = world.Character();
    const CpuTextureResource* playerTexture = textureFor(set.player);
    if (playerTexture == nullptr || !QueueSprite(batch, *playerTexture, player.x, player.z, farm.Width(), farm.Height(), 0.17F, 30, 0)) return Fail(FarmSpriteRenderError::QueueRejected);
    SoftwareRenderer candidateRenderer;
    RenderCamera camera;
    const float aspect = static_cast<float>(farm.Width()) / static_cast<float>(farm.Height());
    if (!candidateRenderer.Initialize(renderer.Width(), renderer.Height()) || !candidateRenderer.Clear(kBackground) || !camera.Initialize({RenderCameraMode::Orthographic, {}, static_cast<float>(farm.Height()) * 0.5F, 60.0F, aspect, 0.1F, 10.0F}) || !batch.Flush(candidateRenderer, camera)) return Fail(FarmSpriteRenderError::RenderFailed);
    renderer = std::move(candidateRenderer);
    lastError_ = FarmSpriteRenderError::None;
    return true;
}
} // namespace NeoEngine
