#include "FarmRenderAdapter.h"

#include "RenderCamera.h"
#include "SoftwareRenderer.h"
#include "SpriteBatch.h"
#include "Systems/FarmSystem.h"
#include "Systems/FarmWorldTool.h"

#include <algorithm>

namespace NeoEngine {
namespace {

uint32_t TileColor(FarmTileState state) { switch (state) { case FarmTileState::Empty: return 0xFF4E9A51; case FarmTileState::Tilled: return 0xFF75402A; case FarmTileState::Growing: return 0xFF67B83B; case FarmTileState::Harvestable: return 0xFFE1C34C; } return 0xFFFF00FF; }
uint32_t BuildingColor(FarmBuildingType type) { switch (type) { case FarmBuildingType::Farmhouse: return 0xFFF0E1C2; case FarmBuildingType::Barn: return 0xFFC94C4C; case FarmBuildingType::Silo: return 0xFFB8C5D0; case FarmBuildingType::Market: return 0xFFF3B54A; case FarmBuildingType::Workshop: return 0xFF8E6B4F; case FarmBuildingType::TownHall: return 0xFF7659B5; } return 0xFFFF00FF; }
uint32_t NpcColor(FarmNpcRole role) { switch (role) { case FarmNpcRole::Farmer: return 0xFF2E86DE; case FarmNpcRole::Builder: return 0xFFE67E22; case FarmNpcRole::Merchant: return 0xFFF1C40F; case FarmNpcRole::QuestGiver: return 0xFF9B59B6; case FarmNpcRole::Ranger: return 0xFF16A085; } return 0xFFFF00FF; }
bool QueueMarker(SpriteBatch& batch, uint16_t x, uint16_t z, uint16_t width, uint16_t height, float scale, int16_t layer, uint32_t color) { if (width == 0 || height == 0 || x >= width || z >= height) return false; const float worldX = static_cast<float>(x) + 0.5F - static_cast<float>(width) * 0.5F; const float worldY = static_cast<float>(z) + 0.5F - static_cast<float>(height) * 0.5F; return batch.Queue({worldX, worldY, 1.0F, 2.0F * scale, 2.0F * scale, layer, 0, color}); }

} // namespace

bool FarmRenderAdapter::Render(const FarmSystem& farm, SoftwareRenderer& renderer) { if (!farm.IsReady() || !renderer.Clear(0xFF17324D)) return false; const float dx = 2.0F / farm.Width(), dz = 2.0F / farm.Height(); for (uint16_t z = 0; z < farm.Height(); ++z) for (uint16_t x = 0; x < farm.Width(); ++x) { const float left = -1.0F + dx * x, right = left + dx, bottom = -1.0F + dz * z, top = bottom + dz; const uint32_t color = TileColor(farm.TileStateAt(x, z)); const RenderVertex a{left,bottom,0.0F,color}, b{right,bottom,0.0F,color}, c{right,top,0.0F,color}, d{left,top,0.0F,color}; if (!renderer.DrawTriangle(a,b,c) || !renderer.DrawTriangle(a,c,d)) return false; } return true; }
bool FarmRenderAdapter::RenderWorldTiles(const FarmSystem& farm, SoftwareRenderer& renderer, RenderCamera& camera) { if (!farm.IsReady() || static_cast<uint64_t>(farm.Width()) * farm.Height() > SpriteBatch::kMaxSprites) return false; SpriteBatch batch; for(uint16_t z=0;z<farm.Height();++z) for(uint16_t x=0;x<farm.Width();++x) if(!QueueMarker(batch,x,z,farm.Width(),farm.Height(),0.5F,0,TileColor(farm.TileStateAt(x,z)))) return false; return batch.Flush(renderer,camera); }
bool FarmRenderAdapter::RenderWorldActors(const FarmSystem& farm, const FarmWorldTool& world, SoftwareRenderer& renderer, RenderCamera& camera) { if (!world.IsReady() || world.Snapshot().worldWidth != farm.Width() || world.Snapshot().worldHeight != farm.Height()) return false; SpriteBatch batch; for (const FarmWorldBuilding& building : world.Buildings()) if (!QueueMarker(batch, building.x, building.z, farm.Width(), farm.Height(), 0.42F, 10, BuildingColor(building.type))) return false; for (const FarmWorldNpc& npc : world.Npcs()) if (!QueueMarker(batch, npc.x, npc.z, farm.Width(), farm.Height(), 0.22F, 20, NpcColor(npc.role))) return false; const FarmCharacterState& character = world.Character(); return QueueMarker(batch, character.x, character.z, farm.Width(), farm.Height(), 0.17F, 30, 0xFFFFFFFF) && batch.Flush(renderer, camera); }
bool FarmRenderAdapter::RenderWorld(const FarmSystem& farm, const FarmWorldTool& world, SoftwareRenderer& renderer) { if (!farm.IsReady() || !renderer.Clear(0xFF17324D)) return false; RenderCamera camera; const float aspect = static_cast<float>(farm.Width()) / static_cast<float>(farm.Height()); if (!camera.Initialize({RenderCameraMode::Orthographic, {}, static_cast<float>(farm.Height()) * 0.5F, 60.0F, aspect, 0.1F, 10.0F})) return false; return RenderWorldTiles(farm,renderer,camera) && RenderWorldActors(farm,world,renderer,camera); }

} // namespace NeoEngine
