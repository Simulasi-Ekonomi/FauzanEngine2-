#include "Runtime/FarmRuntimeHud.h"

#include "Runtime/SoftwareRenderer.h"
#include "Runtime/UiCanvasRenderer.h"

#include <string>

namespace NeoEngine {
bool FarmRuntimeHud::Draw(const FarmRuntimeFrameReceipt& receipt, SoftwareRenderer& renderer) {
    if (receipt.frame == 0U || receipt.framebufferHash == 0U || renderer.Width() < 64U || renderer.Height() < 48U) { lastError_ = FarmRuntimeHudError::InvalidReceipt; return false; }
    UiInputRouter router; UiCanvasRenderer canvas;
    const auto add = [&router, &canvas](uint16_t id, float y, std::string text) { return router.AddWidget({id, 0U, {2.0F, y, 58.0F, 12.0F}, static_cast<int16_t>(id), false, true, false}) && canvas.SetStyle({id, 0xD0202020U}) && canvas.SetLabel({id, std::move(text), 2U, 2U, 1U, 0xFFFFFFFFU}); };
    if (!add(1U, 2.0F, "FRAME " + std::to_string(receipt.frame)) || !add(2U, 16.0F, "COINS " + std::to_string(receipt.telemetry.coins)) || !add(3U, 30.0F, "TICK " + std::to_string(receipt.telemetry.simulationTick))) { lastError_ = FarmRuntimeHudError::SetupFailed; return false; }
    SoftwareRenderer candidate = renderer; if (!canvas.Draw(router, candidate)) { lastError_ = FarmRuntimeHudError::DrawFailed; return false; }
    renderer = std::move(candidate); lastError_ = FarmRuntimeHudError::None; return true;
}
} // namespace NeoEngine
