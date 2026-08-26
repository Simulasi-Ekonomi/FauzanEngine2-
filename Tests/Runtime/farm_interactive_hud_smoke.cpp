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
std::vector<uint8_t> Ppm(uint8_t red,uint8_t green,uint8_t blue){return {'P','6','\n','1',' ','1','\n','2','5','5','\n',red,green,blue};}
bool BindInput(NeoEngine::InputState& input){return input.Bind("farm_move_up",kUp)&&input.Bind("farm_move_down",kDown)&&input.Bind("farm_move_left",kLeft)&&input.Bind("farm_move_right",kRight)&&input.Bind("farm_interact",kInteract);}
}
int main(){
    using namespace NeoEngine;
    const std::array<std::string,16> ids{"tile.empty","tile.tilled","tile.growing","tile.harvestable","building.farmhouse","building.barn","building.silo","building.market","building.workshop","building.townhall","npc.farmer","npc.builder","npc.merchant","npc.quest","npc.ranger","player"};
    AssetRegistry assets;for(size_t index=0U;index<ids.size();++index)if(!assets.ImportBytes(ids[index],AssetKind::Texture,{},Ppm(static_cast<uint8_t>(20U+index*7U),static_cast<uint8_t>(40U+index*5U),static_cast<uint8_t>(60U+index*3U)))||!assets.MarkReady(ids[index]))return 1;
    FarmSystem farm(4U,4U,100);TrustSafetySystem trust;FarmWorldTool world;FarmWorldConfig config{};config.worldWidth=4U;config.worldHeight=4U;config.npcCount=1U;if(!world.Initialize(farm,trust,"interactive-hud-player",config)||!world.SetCharacterState({1U,1U,2U}))return 1;
    FarmSpriteAssetSet set{ids[0],ids[1],ids[2],ids[3],ids[4],ids[5],ids[6],ids[7],ids[8],ids[9],ids[10],ids[11],ids[12],ids[13],ids[14],ids[15]};TextureStagingStore textures;SoftwareRenderer renderer;InputState input;FarmRuntimeSession session;FarmRuntimeHud hud;
    if(!renderer.Initialize(128U,96U)||!BindInput(input)||!session.Initialize(farm,world,set,assets,textures,renderer))return 1;
    FarmActionPanelReceipt preserved{99U,FarmPlayerAction::Harvest,true};if(session.RouteHudPointer(hud,75.0F,22.0F,UiPointerPhase::Press,preserved)||session.LastError()!=FarmRuntimeSessionError::HudInputRejected||preserved.activatedWidget!=99U||session.InputBridge().SelectedAction()!=FarmPlayerAction::Till)return 1;
    if(!session.Frame(input))return 1;const uint64_t worldHash=renderer.FrameHash();FarmRuntimeHudReceipt overlay{};if(!session.DrawHud(hud,overlay)||overlay.worldFramebufferHash!=worldHash||overlay.hudFramebufferHash==worldHash||overlay.inventory.wheatSeeds!=32U||overlay.inventory.wheatProduce!=0U||session.InputBridge().SelectedAction()!=FarmPlayerAction::Till)return 1;
    if(!input.Push(kInteract,true)||!session.Frame(input)||session.LastFrameReceipt().telemetry.tilledTiles!=1U)return 1;
    if(!input.Push(kInteract,false)||!session.Frame(input)||session.LastFrameReceipt().telemetry.tilledTiles!=1U)return 1;
    FarmActionPanelReceipt selection{};if(!session.DrawHud(hud,overlay)||!session.RouteHudPointer(hud,75.0F,22.0F,UiPointerPhase::Press,selection)||selection.selected||!session.RouteHudPointer(hud,127.0F,95.0F,UiPointerPhase::Release,selection)||!selection.selected||selection.activatedWidget!=12U||selection.selectedAction!=FarmPlayerAction::PlantWheat||session.InputBridge().SelectedAction()!=FarmPlayerAction::PlantWheat)return 1;
    if(!input.Push(kInteract,true)||!session.Frame(input)||session.LastFrameReceipt().telemetry.growingTiles!=1U||session.LastFrameReceipt().inventory.wheatSeeds!=31U)return 1;const uint64_t plantedWorldHash=renderer.FrameHash();
    if(!session.DrawHud(hud,overlay)||overlay.hudFramebufferHash==plantedWorldHash||overlay.inventory.wheatSeeds!=31U||overlay.telemetry.growingTiles!=1U||session.FrameCount()!=4U)return 1;
    std::printf("FARM_INTERACTIVE_HUD_SMOKE_OK frame=%llu hud=1 action=plant canonicalInteract=1 tilled=%u growing=%u wheatSeeds=%u\n",static_cast<unsigned long long>(overlay.frame),overlay.telemetry.tilledTiles,overlay.telemetry.growingTiles,overlay.inventory.wheatSeeds);return 0;
}
