#pragma once

#include "FarmPlayerInputBridge.h"
#include "UiInputRouter.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {

enum class FarmActionPanelError : uint8_t { None, NotInitialized, InvalidConfiguration, DuplicateWidget, DuplicateAction, UnknownWidget, ActionUnavailable, BridgeNotReady, RouterRejected };

struct FarmActionPanelBinding { uint16_t widgetId = 0U; FarmPlayerAction action = FarmPlayerAction::Till; };
struct FarmActionPanelReceipt { uint16_t activatedWidget = 0U; FarmPlayerAction selectedAction = FarmPlayerAction::Till; bool selected = false; };

// Bounded UI-selection adapter. It never calls FarmWorldTool or ticks a Farm
// session; it only selects the already-canonical action used by the local
// FarmPlayerInputBridge on a later interact input.
class FarmActionPanelController {
public:
    static constexpr uint8_t kActionCount = 4U;

    bool Initialize(const std::vector<FarmActionPanelBinding>& bindings);
    void SetAvailability(FarmActionAvailability availability) { availability_ = availability; }
    bool SelectWidget(uint16_t widgetId, FarmPlayerInputBridge& bridge, FarmActionPanelReceipt& receipt);
    bool RoutePointer(UiInputRouter& router, float x, float y, UiPointerPhase phase, FarmPlayerInputBridge& bridge, FarmActionPanelReceipt& receipt);
    bool RouteKeyboard(UiInputRouter& router, UiKeyboardKey key, FarmPlayerInputBridge& bridge, FarmActionPanelReceipt& receipt);
    [[nodiscard]] FarmActionPanelError LastError() const { return lastError_; }
    [[nodiscard]] bool IsReady() const { return initialized_; }

private:
    [[nodiscard]] const FarmActionPanelBinding* FindBinding(uint16_t widgetId) const;
    [[nodiscard]] bool IsAvailableWidget(uint16_t widgetId) const;
    bool ApplySelection(uint16_t widgetId, FarmPlayerInputBridge& bridge, FarmActionPanelReceipt& receipt);
    std::vector<FarmActionPanelBinding> bindings_;
    FarmActionAvailability availability_{true, true, true, true};
    FarmActionPanelError lastError_ = FarmActionPanelError::NotInitialized;
    bool initialized_ = false;
};

} // namespace NeoEngine
