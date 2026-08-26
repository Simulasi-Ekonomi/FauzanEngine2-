#include "Runtime/FarmRuntimeHud.h"
#include "Runtime/SoftwareRenderer.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    SoftwareRenderer renderer; if (!renderer.Initialize(64U,48U) || !renderer.Clear(0xFF000000U)) return 1;
    FarmRuntimeHud hud; FarmRuntimeFrameReceipt invalid{}; const uint64_t black=renderer.FrameHash();
    if (hud.Draw(invalid,renderer) || hud.LastError()!=FarmRuntimeHudError::InvalidReceipt || renderer.FrameHash()!=black) return 1;
    FarmRuntimeFrameReceipt receipt{}; receipt.frame=7U; receipt.framebufferHash=123U; receipt.telemetry.simulationTick=9U; receipt.telemetry.coins=100;
    if (!hud.Draw(receipt,renderer) || hud.LastError()!=FarmRuntimeHudError::None || renderer.FrameHash()==black) return 1;
    const uint64_t hudHash=renderer.FrameHash(); receipt.framebufferHash=0U;
    if (hud.Draw(receipt,renderer) || hud.LastError()!=FarmRuntimeHudError::InvalidReceipt || renderer.FrameHash()!=hudHash) return 1;
    std::printf("FARM_RUNTIME_HUD_SMOKE_OK hud=1 atomic=1 hash=%llu\n", static_cast<unsigned long long>(hudHash)); return 0;
}
