#pragma once

#include "FarmActionPanelController.h"
#include "FarmRuntimeSession.h"
#include "UiCanvasRenderer.h"
#include "UiLayoutResolver.h"

#include <cstdint>
#include <string_view>

namespace NeoEngine {
class AssetRegistry;
struct CpuTextureResource;
class SoftwareRenderer;
class TextureStagingStore;
enum class FarmRuntimeHudError : uint8_t { None, InvalidReceipt, SetupFailed, DrawFailed, InputUnavailable, InputRejected };
class FarmRuntimeHud {
public:
    bool Draw(const FarmRuntimeFrameReceipt& receipt, SoftwareRenderer& renderer);
    bool Draw(const FarmRuntimeFrameReceipt& receipt, FarmPlayerAction selectedAction, SoftwareRenderer& renderer);
    bool Draw(const FarmRuntimeFrameReceipt& receipt, FarmPlayerAction selectedAction, const AssetRegistry& registry, const TextureStagingStore& textures, std::string_view panelIconAsset, SoftwareRenderer& renderer);
    void SetActionAvailability(FarmActionAvailability availability) { actionPanel_.SetAvailability(availability); availability_ = availability; }
    bool RoutePointer(float x, float y, UiPointerPhase phase, FarmPlayerInputBridge& bridge, FarmActionPanelReceipt& receipt);
    bool RouteKeyboard(UiKeyboardKey key, FarmPlayerInputBridge& bridge, FarmActionPanelReceipt& receipt);
    [[nodiscard]] FarmRuntimeHudError LastError() const { return lastError_; }
private:
    bool EnsureLayout(SoftwareRenderer& renderer);
    bool ConfigureCanvas(const FarmRuntimeFrameReceipt& receipt, FarmPlayerAction selectedAction, const AssetRegistry* registry, const CpuTextureResource* panelIcon, UiCanvasRenderer& canvas) const;
    UiInputRouter router_{};
    UiLayoutResolver layout_{};
    FarmActionPanelController actionPanel_{};
    uint32_t layoutWidth_ = 0U;
    uint32_t layoutHeight_ = 0U;
    bool interactive_ = false;
    bool configured_ = false;
    FarmActionAvailability availability_{true, true, true, true};
    FarmRuntimeHudError lastError_ = FarmRuntimeHudError::None;
};
} // namespace NeoEngine
