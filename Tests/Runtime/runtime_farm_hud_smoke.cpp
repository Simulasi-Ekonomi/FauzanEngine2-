#include "Runtime/NeoRuntime.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    RuntimeConfig invalid{}; invalid.renderWidth = 63U; invalid.renderHeight = 48U; invalid.enableFarmRuntimeHud = true; NeoRuntime rejected;
    if (rejected.Initialize(invalid) || rejected.LastError() != RuntimeError::InvalidConfiguration) return 1;
    RuntimeConfig base{}; base.renderWidth = 64U; base.renderHeight = 48U; NeoRuntime core;
    if (!core.Initialize(base) || !core.Tick() || !core.RenderFarm() || core.Renderer() == nullptr || core.Renderer()->FrameHash() == 0U || core.LastFarmRenderReceipt() == nullptr || core.LastFarmRenderReceipt()->frame != 1U || core.LastFarmRenderReceipt()->worldFramebufferHash != core.Renderer()->FrameHash() || core.LastFarmRenderReceipt()->hudFramebufferHash != 0U || core.LastFarmRenderReceipt()->presentedFrameCount != 0U || core.LastFarmRenderReceipt()->telemetry.simulationTick != 1U) return 1;
    const uint64_t worldHash = core.Renderer()->FrameHash();
    RuntimeConfig overlay = base; overlay.enableFarmRuntimeHud = true; NeoRuntime hud;
    if (!hud.Initialize(overlay) || !hud.Tick() || !hud.RenderFarm() || hud.LastError() != RuntimeError::None || hud.Renderer() == nullptr || hud.Renderer()->FrameHash() == 0U || hud.Renderer()->FrameHash() == worldHash || hud.LastFarmRenderReceipt() == nullptr || hud.LastFarmRenderReceipt()->frame != 1U || hud.LastFarmRenderReceipt()->worldFramebufferHash == 0U || hud.LastFarmRenderReceipt()->hudFramebufferHash != hud.Renderer()->FrameHash() || hud.LastFarmRenderReceipt()->hudFramebufferHash == hud.LastFarmRenderReceipt()->worldFramebufferHash || hud.LastFarmRenderReceipt()->presentedFrameCount != 0U || hud.LastFarmRenderReceipt()->telemetry.simulationTick != 1U) return 1;
    const uint64_t hudHash = hud.Renderer()->FrameHash();
    if (!hud.RenderFarm() || hud.Renderer()->FrameHash() == 0U || hud.LastFarmRenderReceipt() == nullptr || hud.LastFarmRenderReceipt()->frame != 2U) return 1;
    RuntimeConfig presentedConfig = base; presentedConfig.enableSoftwareSurfacePresentation = true; presentedConfig.softwareSurfaceHidden = true; NeoRuntime presented;
    if (!presented.Initialize(presentedConfig) || !presented.Tick() || !presented.RenderFarm() || presented.SurfacePresenter() == nullptr || presented.SurfacePresenter()->PresentedFrameCount() != 1U || presented.LastFarmRenderReceipt() == nullptr || presented.LastFarmRenderReceipt()->presentedFrameCount != 1U || presented.LastFarmRenderReceipt()->worldFramebufferHash != presented.Renderer()->FrameHash() || !core.Shutdown() || !hud.Shutdown() || !presented.Shutdown()) return 1;
    std::printf("RUNTIME_FARM_HUD_SMOKE_OK world=%llu hud=%llu overlay=1\n", static_cast<unsigned long long>(worldHash), static_cast<unsigned long long>(hudHash));
    return 0;
}
