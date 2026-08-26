#include "Demos/FarmInteractiveSurfaceDemo.h"

#include "Runtime/AssetRegistry.h"
#include "Runtime/FarmRuntimeHud.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SoftwareSurfacePresenter.h"
#include "Runtime/TextureStaging.h"
#include "Systems/FarmSystem.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <array>
#include <vector>

namespace NeoEngine {
namespace {

constexpr int32_t kUp = 1, kDown = 2, kLeft = 3, kRight = 4, kInteract = 5;
std::vector<uint8_t> Ppm(uint8_t red,uint8_t green,uint8_t blue){return {'P','6','\n','1',' ','1','\n','2','5','5','\n',red,green,blue};}
bool BindInput(InputState& input){return input.Bind("farm_move_up",kUp)&&input.Bind("farm_move_down",kDown)&&input.Bind("farm_move_left",kLeft)&&input.Bind("farm_move_right",kRight)&&input.Bind("farm_interact",kInteract);}

} // namespace

bool RunFarmInteractiveSurfaceDemo(const FarmInteractiveSurfaceDemoConfig& config, FarmInteractiveSurfaceDemoReceipt& receipt, FarmInteractiveSurfaceDemoError& error) {
    error = FarmInteractiveSurfaceDemoError::None;
    if (config.width < 128U || config.height < 96U || config.width > 1024U || config.height > 1024U || config.ppmPath.empty() || config.ppmPath.size() > 256U) { error = FarmInteractiveSurfaceDemoError::InvalidConfiguration; return false; }
    const std::array<std::string,16> ids{"tile.empty","tile.tilled","tile.growing","tile.harvestable","building.farmhouse","building.barn","building.silo","building.market","building.workshop","building.townhall","npc.farmer","npc.builder","npc.merchant","npc.quest","npc.ranger","player"};
    AssetRegistry assets;
    for (size_t index=0U; index<ids.size(); ++index) if (!assets.ImportBytes(ids[index],AssetKind::Texture,{},Ppm(static_cast<uint8_t>(20U+index*7U),static_cast<uint8_t>(40U+index*5U),static_cast<uint8_t>(60U+index*3U))) || !assets.MarkReady(ids[index])) { error = FarmInteractiveSurfaceDemoError::AssetSetupFailed; return false; }
    FarmSystem farm(4U,4U,100); TrustSafetySystem trust; FarmWorldTool world; FarmWorldConfig worldConfig{}; worldConfig.worldWidth=4U; worldConfig.worldHeight=4U; worldConfig.npcCount=1U;
    FarmSpriteAssetSet spriteSet{ids[0],ids[1],ids[2],ids[3],ids[4],ids[5],ids[6],ids[7],ids[8],ids[9],ids[10],ids[11],ids[12],ids[13],ids[14],ids[15]};
    TextureStagingStore textures; SoftwareRenderer renderer; InputState input; FarmRuntimeSession session; FarmRuntimeHud hud; SoftwareSurfacePresenter presenter;
    if (!world.Initialize(farm,trust,"interactive-surface-player",worldConfig) || !world.SetCharacterState({1U,1U,2U}) || !renderer.Initialize(config.width,config.height) || !BindInput(input) || !session.Initialize(farm,world,spriteSet,assets,textures,renderer) || !presenter.Initialize({config.width,config.height,config.hiddenSurface})) { presenter.Reset(); error = FarmInteractiveSurfaceDemoError::RuntimeSetupFailed; return false; }
    FarmRuntimeHudReceipt hudReceipt{};
    const auto frameAndPresent = [&]() {
        if (!session.Frame(input) || !session.DrawHud(hud,hudReceipt)) return false;
        return presenter.PumpEvents() && presenter.Present(renderer);
    };
    if (!frameAndPresent()) { presenter.Reset(); error = FarmInteractiveSurfaceDemoError::FrameFailed; return false; }
    if (!input.Push(kInteract,true) || !frameAndPresent() || session.LastFrameReceipt().telemetry.tilledTiles != 1U) { presenter.Reset(); error = FarmInteractiveSurfaceDemoError::FrameFailed; return false; }
    if (!input.Push(kInteract,false) || !frameAndPresent()) { presenter.Reset(); error = FarmInteractiveSurfaceDemoError::FrameFailed; return false; }
    FarmActionPanelReceipt actionReceipt{};
    if (!session.RouteHudPointer(hud,75.0F,22.0F,UiPointerPhase::Press,actionReceipt) || !session.RouteHudPointer(hud,static_cast<float>(config.width-1U),static_cast<float>(config.height-1U),UiPointerPhase::Release,actionReceipt) || !actionReceipt.selected || actionReceipt.selectedAction != FarmPlayerAction::PlantWheat) { presenter.Reset(); error = FarmInteractiveSurfaceDemoError::HudInputFailed; return false; }
    if (!input.Push(kInteract,true) || !frameAndPresent() || session.LastFrameReceipt().telemetry.growingTiles != 1U || session.LastFrameReceipt().inventory.wheatSeeds != 31U || !renderer.WritePpm(config.ppmPath)) { presenter.Reset(); error = FarmInteractiveSurfaceDemoError::ArtifactWriteFailed; return false; }
    const FarmRuntimeFrameReceipt worldReceipt = session.LastFrameReceipt();
    const FarmInteractiveSurfaceDemoReceipt candidate{session.FrameCount(),presenter.PresentedFrameCount(),worldReceipt.framebufferHash,hudReceipt.hudFramebufferHash,session.InputBridge().SelectedAction(),worldReceipt.telemetry,worldReceipt.inventory};
    if (candidate.frames != 4U || candidate.presentedFrames != candidate.frames || candidate.worldFramebufferHash == 0U || candidate.hudFramebufferHash == candidate.worldFramebufferHash || presenter.LastPresentedHash() != candidate.hudFramebufferHash) { presenter.Reset(); error = FarmInteractiveSurfaceDemoError::PresentFailed; return false; }
    presenter.Reset(); receipt = candidate; return true;
}

} // namespace NeoEngine
