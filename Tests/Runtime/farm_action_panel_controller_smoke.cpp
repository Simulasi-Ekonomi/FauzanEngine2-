#include "Runtime/FarmActionPanelController.h"

#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    FarmActionPanelController panel;
    FarmPlayerInputBridge bridge;
    FarmActionPanelReceipt receipt{99U, FarmPlayerAction::Harvest, true};
    if (panel.SelectWidget(2U, bridge, receipt) || panel.LastError() != FarmActionPanelError::NotInitialized || receipt.activatedWidget != 99U) return 1;
    const std::vector<FarmActionPanelBinding> valid{{2U, FarmPlayerAction::Till}, {3U, FarmPlayerAction::PlantWheat}, {4U, FarmPlayerAction::Water}, {5U, FarmPlayerAction::Harvest}};
    if (!panel.Initialize(valid) || !bridge.Initialize() || bridge.SelectedAction() != FarmPlayerAction::Till) return 1;
    if (panel.Initialize({{2U, FarmPlayerAction::Till}, {2U, FarmPlayerAction::PlantWheat}, {4U, FarmPlayerAction::Water}, {5U, FarmPlayerAction::Harvest}}) || panel.LastError() != FarmActionPanelError::DuplicateWidget || bridge.SelectedAction() != FarmPlayerAction::Till || !panel.IsReady()) return 1;
    if (panel.Initialize({{2U, FarmPlayerAction::Till}, {3U, FarmPlayerAction::Till}, {4U, FarmPlayerAction::Water}, {5U, FarmPlayerAction::Harvest}}) || panel.LastError() != FarmActionPanelError::DuplicateAction || bridge.SelectedAction() != FarmPlayerAction::Till) return 1;
    if (panel.SelectWidget(99U, bridge, receipt) || panel.LastError() != FarmActionPanelError::UnknownWidget || bridge.SelectedAction() != FarmPlayerAction::Till || receipt.activatedWidget != 99U) return 1;
    if (!panel.SelectWidget(3U, bridge, receipt) || !receipt.selected || receipt.activatedWidget != 3U || receipt.selectedAction != FarmPlayerAction::PlantWheat || bridge.SelectedAction() != FarmPlayerAction::PlantWheat) return 1;

    UiInputRouter router;
    if (!router.AddWidget({1U, 0U, {0, 0, 64, 64}, 0, false, true, false}) || !router.AddWidget({2U, 1U, {2, 2, 20, 10}, 1, true, true, true}) || !router.AddWidget({3U, 1U, {2, 16, 20, 10}, 2, true, true, true}) || !router.AddWidget({4U, 1U, {2, 30, 20, 10}, 3, true, true, true}) || !router.AddWidget({5U, 1U, {2, 44, 20, 10}, 4, true, true, true}) || !router.AddWidget({6U, 1U, {30, 2, 20, 10}, 5, true, true, true})) return 1;
    if (!panel.RoutePointer(router, 8, 34, UiPointerPhase::Press, bridge, receipt) || receipt.selected || bridge.SelectedAction() != FarmPlayerAction::PlantWheat || !panel.RoutePointer(router, 63, 63, UiPointerPhase::Release, bridge, receipt) || !receipt.selected || receipt.activatedWidget != 4U || receipt.selectedAction != FarmPlayerAction::Water || bridge.SelectedAction() != FarmPlayerAction::Water) return 1;
    if (!panel.RoutePointer(router, 34, 6, UiPointerPhase::Press, bridge, receipt) || !panel.RoutePointer(router, 34, 6, UiPointerPhase::Release, bridge, receipt) || receipt.selected || bridge.SelectedAction() != FarmPlayerAction::Water) return 1;
    if (!router.SetFocus(5U) || !panel.RouteKeyboard(router, UiKeyboardKey::Activate, bridge, receipt) || !receipt.selected || receipt.activatedWidget != 5U || receipt.selectedAction != FarmPlayerAction::Harvest || bridge.SelectedAction() != FarmPlayerAction::Harvest) return 1;
    const FarmActionPanelReceipt preserved = receipt;
    if (panel.RouteKeyboard(router, static_cast<UiKeyboardKey>(99), bridge, receipt) || panel.LastError() != FarmActionPanelError::RouterRejected || bridge.SelectedAction() != FarmPlayerAction::Harvest || receipt.activatedWidget != preserved.activatedWidget || receipt.selectedAction != preserved.selectedAction || receipt.selected != preserved.selected) return 1;
    panel.SetAvailability({false, false, false, false});
    const FarmActionPanelReceipt unavailable = receipt;
    if (panel.SelectWidget(2U, bridge, receipt) || panel.LastError() != FarmActionPanelError::ActionUnavailable || bridge.SelectedAction() != FarmPlayerAction::Harvest || receipt.activatedWidget != unavailable.activatedWidget || receipt.selectedAction != unavailable.selectedAction || receipt.selected != unavailable.selected) return 1;
    if (panel.RoutePointer(router, 8, 6, UiPointerPhase::Press, bridge, receipt) || panel.LastError() != FarmActionPanelError::ActionUnavailable || router.CapturedWidget() != 0U || router.FocusedWidget() != 5U || bridge.SelectedAction() != FarmPlayerAction::Harvest || receipt.activatedWidget != unavailable.activatedWidget) return 1;
    if (panel.RouteKeyboard(router, UiKeyboardKey::Activate, bridge, receipt) || panel.LastError() != FarmActionPanelError::ActionUnavailable || router.FocusedWidget() != 5U || bridge.SelectedAction() != FarmPlayerAction::Harvest || receipt.activatedWidget != unavailable.activatedWidget) return 1;
    std::printf("FARM_ACTION_PANEL_CONTROLLER_SMOKE_OK bindings=4 pointer=1 keyboard=1 availability=1 selection=1 ignored=1 atomic=1\n");
    return 0;
}
