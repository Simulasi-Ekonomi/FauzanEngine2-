#include "FarmActionPanelController.h"

#include <algorithm>

namespace NeoEngine {
namespace {

bool IsKnownAction(FarmPlayerAction action) {
    switch (action) {
        case FarmPlayerAction::Till:
        case FarmPlayerAction::PlantWheat:
        case FarmPlayerAction::Water:
        case FarmPlayerAction::Harvest: return true;
    }
    return false;
}

} // namespace

const FarmActionPanelBinding* FarmActionPanelController::FindBinding(uint16_t widgetId) const {
    const auto found = std::find_if(bindings_.begin(), bindings_.end(), [widgetId](const FarmActionPanelBinding& binding) { return binding.widgetId == widgetId; });
    return found == bindings_.end() ? nullptr : &*found;
}

bool FarmActionPanelController::Initialize(const std::vector<FarmActionPanelBinding>& bindings) {
    if (bindings.size() != kActionCount) { lastError_ = FarmActionPanelError::InvalidConfiguration; return false; }
    std::vector<FarmActionPanelBinding> candidate;
    candidate.reserve(bindings.size());
    for (const FarmActionPanelBinding& binding : bindings) {
        if (binding.widgetId == 0U || !IsKnownAction(binding.action)) { lastError_ = FarmActionPanelError::InvalidConfiguration; return false; }
        if (std::any_of(candidate.begin(), candidate.end(), [&binding](const FarmActionPanelBinding& prior) { return prior.widgetId == binding.widgetId; })) { lastError_ = FarmActionPanelError::DuplicateWidget; return false; }
        if (std::any_of(candidate.begin(), candidate.end(), [&binding](const FarmActionPanelBinding& prior) { return prior.action == binding.action; })) { lastError_ = FarmActionPanelError::DuplicateAction; return false; }
        candidate.push_back(binding);
    }
    bindings_ = std::move(candidate);
    initialized_ = true;
    lastError_ = FarmActionPanelError::None;
    return true;
}

bool FarmActionPanelController::ApplySelection(uint16_t widgetId, FarmPlayerInputBridge& bridge, FarmActionPanelReceipt& receipt) {
    if (!initialized_) { lastError_ = FarmActionPanelError::NotInitialized; return false; }
    const FarmActionPanelBinding* binding = FindBinding(widgetId);
    if (binding == nullptr) { lastError_ = FarmActionPanelError::UnknownWidget; return false; }
    if (!bridge.IsReady()) { lastError_ = FarmActionPanelError::BridgeNotReady; return false; }
    FarmActionPanelReceipt candidate{widgetId, binding->action, true};
    bridge.SetSelectedAction(binding->action);
    receipt = candidate;
    lastError_ = FarmActionPanelError::None;
    return true;
}

bool FarmActionPanelController::SelectWidget(uint16_t widgetId, FarmPlayerInputBridge& bridge, FarmActionPanelReceipt& receipt) { return ApplySelection(widgetId, bridge, receipt); }

bool FarmActionPanelController::RoutePointer(UiInputRouter& router, float x, float y, UiPointerPhase phase, FarmPlayerInputBridge& bridge, FarmActionPanelReceipt& receipt) {
    if (!initialized_) { lastError_ = FarmActionPanelError::NotInitialized; return false; }
    const UiPointerResult routed = router.RoutePointer(x, y, phase);
    if (phase != UiPointerPhase::Release || routed.targetId == 0U || FindBinding(routed.targetId) == nullptr) {
        receipt = {0U, bridge.SelectedAction(), false};
        lastError_ = FarmActionPanelError::None;
        return true;
    }
    return ApplySelection(routed.targetId, bridge, receipt);
}

bool FarmActionPanelController::RouteKeyboard(UiInputRouter& router, UiKeyboardKey key, FarmPlayerInputBridge& bridge, FarmActionPanelReceipt& receipt) {
    if (!initialized_) { lastError_ = FarmActionPanelError::NotInitialized; return false; }
    const UiKeyboardResult routed = router.RouteKeyboard(key);
    if (router.LastError() != UiError::None) { lastError_ = FarmActionPanelError::RouterRejected; return false; }
    if (!routed.activated || routed.targetId == 0U || FindBinding(routed.targetId) == nullptr) {
        receipt = {0U, bridge.SelectedAction(), false};
        lastError_ = FarmActionPanelError::None;
        return true;
    }
    return ApplySelection(routed.targetId, bridge, receipt);
}

} // namespace NeoEngine
