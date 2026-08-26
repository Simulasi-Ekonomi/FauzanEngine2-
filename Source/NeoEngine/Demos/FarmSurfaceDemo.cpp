#include "Demos/FarmSurfaceDemo.h"

#include "Runtime/NeoRuntime.h"

namespace NeoEngine {
bool RunFarmSurfaceDemo(const FarmSurfaceDemoConfig& config, FarmSurfaceDemoReceipt& receipt, FarmSurfaceDemoError& error) {
    error = FarmSurfaceDemoError::None;
    if (config.width < 32U || config.height < 32U || config.width > 1024U || config.height > 1024U || config.frames == 0U || config.frames > 600U || config.ppmPath.empty() || config.ppmPath.size() > 256U) { error = FarmSurfaceDemoError::InvalidConfiguration; return false; }
    NeoRuntime runtime;
    RuntimeConfig runtimeConfig{};
    runtimeConfig.farmWidth = 8;
    runtimeConfig.farmHeight = 8;
    runtimeConfig.renderWidth = static_cast<uint16_t>(config.width);
    runtimeConfig.renderHeight = static_cast<uint16_t>(config.height);
    runtimeConfig.farmNpcCount = 5;
    runtimeConfig.authoringWorldSide = 32;
    runtimeConfig.enableFarmRuntimeHud = true;
    runtimeConfig.enableSoftwareSurfacePresentation = true;
    runtimeConfig.softwareSurfaceHidden = config.hiddenSurface;
    if (!runtime.Initialize(runtimeConfig)) { error = FarmSurfaceDemoError::RuntimeInitializeFailed; return false; }
    FarmWorldTool* world = runtime.FarmWorld();
    if (world == nullptr || !world->SetCharacterState({2, 2, 1}) || !world->SetGovernmentPolicy(FarmGovernmentPolicy::ConstructionPermits, true)) { runtime.Shutdown(); error = FarmSurfaceDemoError::FarmSetupFailed; return false; }
    uint64_t permit = 0;
    uint32_t building = 0;
    if (!world->IssueBuildingPermit(FarmBuildingType::Barn, permit) || !world->PlaceBuilding(permit, 4, 4, building) || !world->PlayerTill(2, 2) || !world->PlayerPlant(2, 2, FarmCrop::Wheat) || !world->PlayerWater(2, 2)) { runtime.Shutdown(); error = FarmSurfaceDemoError::FarmSetupFailed; return false; }
    for (uint32_t frame = 0; frame < config.frames; ++frame) if (!runtime.Tick() || !runtime.RenderFarm()) { runtime.Shutdown(); error = FarmSurfaceDemoError::FrameFailed; return false; }
    SoftwareRenderer* renderer = runtime.Renderer();
    const SoftwareSurfacePresenter* surface = runtime.SurfacePresenter();
    const FarmWorldSnapshot snapshot = world->Snapshot();
    const FarmSystem* farm = runtime.Farm();
    const RuntimeFarmRenderReceipt* runtimeReceipt = runtime.LastFarmRenderReceipt();
    if (renderer == nullptr || surface == nullptr || farm == nullptr || runtimeReceipt == nullptr || surface->PresentedFrameCount() != config.frames || runtimeReceipt->frame != config.frames || runtimeReceipt->presentedFrameCount != config.frames || runtimeReceipt->worldFramebufferHash == 0U || runtimeReceipt->hudFramebufferHash != renderer->FrameHash() || runtimeReceipt->hudFramebufferHash == runtimeReceipt->worldFramebufferHash) { runtime.Shutdown(); error = FarmSurfaceDemoError::ArtifactWriteFailed; return false; }
    const uint64_t worldHash = runtimeReceipt->worldFramebufferHash;
    const uint64_t hudHash = runtimeReceipt->hudFramebufferHash;
    if (!renderer->WritePpm(config.ppmPath)) { runtime.Shutdown(); error = FarmSurfaceDemoError::ArtifactWriteFailed; return false; }
    const FarmSurfaceDemoReceipt candidate{config.frames, hudHash, static_cast<uint32_t>(runtimeReceipt->presentedFrameCount), snapshot.buildings, snapshot.npcs, worldHash, hudHash};
    if (!runtime.Shutdown()) { error = FarmSurfaceDemoError::ShutdownFailed; return false; }
    receipt = candidate;
    return true;
}
} // namespace NeoEngine
