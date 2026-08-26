#include "Runtime/NeoRuntime.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    RuntimeConfig invalid{}; invalid.renderWidth = 63U; invalid.renderHeight = 48U; invalid.enableFarmRuntimeHud = true; NeoRuntime rejected;
    if (rejected.Initialize(invalid) || rejected.LastError() != RuntimeError::InvalidConfiguration) return 1;
    RuntimeConfig base{}; base.renderWidth = 64U; base.renderHeight = 48U; NeoRuntime core;
    if (!core.Initialize(base) || !core.Tick() || !core.RenderFarm() || core.Renderer() == nullptr || core.Renderer()->FrameHash() == 0U) return 1;
    const uint64_t worldHash = core.Renderer()->FrameHash();
    RuntimeConfig overlay = base; overlay.enableFarmRuntimeHud = true; NeoRuntime hud;
    if (!hud.Initialize(overlay) || !hud.Tick() || !hud.RenderFarm() || hud.LastError() != RuntimeError::None || hud.Renderer() == nullptr || hud.Renderer()->FrameHash() == 0U || hud.Renderer()->FrameHash() == worldHash) return 1;
    const uint64_t hudHash = hud.Renderer()->FrameHash();
    if (!hud.RenderFarm() || hud.Renderer()->FrameHash() == 0U || !core.Shutdown() || !hud.Shutdown()) return 1;
    std::printf("RUNTIME_FARM_HUD_SMOKE_OK world=%llu hud=%llu overlay=1\n", static_cast<unsigned long long>(worldHash), static_cast<unsigned long long>(hudHash));
    return 0;
}
