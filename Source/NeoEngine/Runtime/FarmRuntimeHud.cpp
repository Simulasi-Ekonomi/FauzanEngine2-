#include "Runtime/FarmRuntimeHud.h"

#include "Runtime/SoftwareRenderer.h"

#include <string>
#include <vector>

namespace NeoEngine {
namespace { uint32_t ActionColor(FarmPlayerAction selected, FarmPlayerAction action) { return selected == action ? 0xFF2E8B57U : 0xFF304050U; } }
bool FarmRuntimeHud::EnsureLayout(SoftwareRenderer& renderer) {
    if (configured_ && layoutWidth_ == renderer.Width() && layoutHeight_ == renderer.Height()) return true;
    UiInputRouter candidateRouter; UiLayoutResolver candidateLayout; FarmActionPanelController candidatePanel;
    const bool enhanced = renderer.Width() >= 128U && renderer.Height() >= 96U;
    if (!enhanced) {
        if (renderer.Width() < 64U || renderer.Height() < 48U || !candidateRouter.AddWidget({1U, 0U, {2.0F, 2.0F, 58.0F, 12.0F}, 1, false, true, false}) || !candidateRouter.AddWidget({2U, 0U, {2.0F, 16.0F, 58.0F, 12.0F}, 2, false, true, false}) || !candidateRouter.AddWidget({3U, 0U, {2.0F, 30.0F, 58.0F, 12.0F}, 3, false, true, false})) return false;
    } else {
        const std::vector<UiWidgetSpec> widgets{{1U,0U,{0,0,1,1},1,false,true,false},{2U,1U,{0,0,1,1},2,false,true,false},{3U,1U,{0,0,1,1},3,false,true,false},{4U,1U,{0,0,1,1},4,false,true,false},{5U,1U,{0,0,1,1},5,false,true,false},{6U,1U,{0,0,1,1},6,false,true,false},{7U,1U,{0,0,1,1},7,false,true,false},{10U,0U,{0,0,1,1},8,false,true,false},{11U,10U,{0,0,1,1},9,true,true,true},{12U,10U,{0,0,1,1},10,true,true,true},{13U,10U,{0,0,1,1},11,true,true,true},{14U,10U,{0,0,1,1},12,true,true,true}};
        for (const UiWidgetSpec& widget : widgets) if (!candidateRouter.AddWidget(widget)) return false;
        const std::vector<UiLayoutSpec> plan{{1U,UiLayoutAnchor::TopLeft,UiLayoutFlow::None,{2,2,2,2},62,92,62,92,0},{2U,UiLayoutAnchor::TopLeft,UiLayoutFlow::VerticalStack,{2,2,2,2},0,10,20,10,2},{3U,UiLayoutAnchor::TopLeft,UiLayoutFlow::VerticalStack,{2,2,2,2},0,10,20,10,2},{4U,UiLayoutAnchor::TopLeft,UiLayoutFlow::VerticalStack,{2,2,2,2},0,10,20,10,2},{5U,UiLayoutAnchor::TopLeft,UiLayoutFlow::VerticalStack,{2,2,2,2},0,10,20,10,2},{6U,UiLayoutAnchor::TopLeft,UiLayoutFlow::VerticalStack,{2,2,2,2},0,10,20,10,2},{7U,UiLayoutAnchor::TopLeft,UiLayoutFlow::VerticalStack,{2,2,2,2},0,10,20,10,2},{10U,UiLayoutAnchor::TopRight,UiLayoutFlow::None,{2,2,2,2},58,64,58,64,0},{11U,UiLayoutAnchor::TopLeft,UiLayoutFlow::VerticalStack,{2,2,2,2},0,10,20,10,2},{12U,UiLayoutAnchor::TopLeft,UiLayoutFlow::VerticalStack,{2,2,2,2},0,10,20,10,2},{13U,UiLayoutAnchor::TopLeft,UiLayoutFlow::VerticalStack,{2,2,2,2},0,10,20,10,2},{14U,UiLayoutAnchor::TopLeft,UiLayoutFlow::VerticalStack,{2,2,2,2},0,10,20,10,2}};
        if (!candidateLayout.Apply({static_cast<float>(renderer.Width()),static_cast<float>(renderer.Height())},plan,candidateRouter) || !candidatePanel.Initialize({{11U,FarmPlayerAction::Till},{12U,FarmPlayerAction::PlantWheat},{13U,FarmPlayerAction::Water},{14U,FarmPlayerAction::Harvest}})) return false;
    }
    router_ = std::move(candidateRouter); layout_ = std::move(candidateLayout); actionPanel_ = std::move(candidatePanel); layoutWidth_ = renderer.Width(); layoutHeight_ = renderer.Height(); interactive_ = enhanced; configured_ = true; return true;
}
bool FarmRuntimeHud::ConfigureCanvas(const FarmRuntimeFrameReceipt& receipt, FarmPlayerAction selectedAction, UiCanvasRenderer& canvas) const {
    if (!interactive_) return canvas.SetStyle({1U,0xD0202020U}) && canvas.SetLabel({1U,"FRAME "+std::to_string(receipt.frame),2U,2U,1U,0xFFFFFFFFU}) && canvas.SetStyle({2U,0xD0202020U}) && canvas.SetLabel({2U,"COINS "+std::to_string(receipt.telemetry.coins),2U,2U,1U,0xFFFFFFFFU}) && canvas.SetStyle({3U,0xD0202020U}) && canvas.SetLabel({3U,"TICK "+std::to_string(receipt.telemetry.simulationTick),2U,2U,1U,0xFFFFFFFFU});
    const bool labels=canvas.SetLabel({2U,"FRAME "+std::to_string(receipt.frame),2U,2U,1U,0xFFFFFFFFU})&&canvas.SetLabel({3U,"COINS "+std::to_string(receipt.telemetry.coins),2U,2U,1U,0xFFFFFFFFU})&&canvas.SetLabel({4U,"TICK "+std::to_string(receipt.telemetry.simulationTick),2U,2U,1U,0xFFFFFFFFU})&&canvas.SetLabel({5U,"GROW "+std::to_string(receipt.telemetry.growingTiles),2U,2U,1U,0xFFFFFFFFU})&&canvas.SetLabel({6U,"READY "+std::to_string(receipt.telemetry.harvestableTiles),2U,2U,1U,0xFFFFFFFFU})&&canvas.SetLabel({7U,"WHEAT "+std::to_string(receipt.inventory.wheatProduce),2U,2U,1U,0xFFFFFFFFU})&&canvas.SetLabel({11U,"TILL",2U,2U,1U,0xFFFFFFFFU})&&canvas.SetLabel({12U,"PLANT",2U,2U,1U,0xFFFFFFFFU})&&canvas.SetLabel({13U,"WATER",2U,2U,1U,0xFFFFFFFFU})&&canvas.SetLabel({14U,"HARVEST",2U,2U,1U,0xFFFFFFFFU});
    return labels&&canvas.SetStyle({1U,0xD0202020U})&&canvas.SetStyle({2U,0xC0202020U})&&canvas.SetStyle({3U,0xC0202020U})&&canvas.SetStyle({4U,0xC0202020U})&&canvas.SetStyle({5U,0xC0202020U})&&canvas.SetStyle({6U,0xC0202020U})&&canvas.SetStyle({7U,0xC0202020U})&&canvas.SetStyle({10U,0xD0202020U})&&canvas.SetStyle({11U,ActionColor(selectedAction,FarmPlayerAction::Till)})&&canvas.SetStyle({12U,ActionColor(selectedAction,FarmPlayerAction::PlantWheat)})&&canvas.SetStyle({13U,ActionColor(selectedAction,FarmPlayerAction::Water)})&&canvas.SetStyle({14U,ActionColor(selectedAction,FarmPlayerAction::Harvest)});
}
bool FarmRuntimeHud::Draw(const FarmRuntimeFrameReceipt& receipt, SoftwareRenderer& renderer) { return Draw(receipt,FarmPlayerAction::Till,renderer); }
bool FarmRuntimeHud::Draw(const FarmRuntimeFrameReceipt& receipt, FarmPlayerAction selectedAction, SoftwareRenderer& renderer) {
    if (receipt.frame==0U||receipt.framebufferHash==0U||!EnsureLayout(renderer)) { lastError_=receipt.frame==0U||receipt.framebufferHash==0U?FarmRuntimeHudError::InvalidReceipt:FarmRuntimeHudError::SetupFailed; return false; }
    UiCanvasRenderer canvas; if (!ConfigureCanvas(receipt,selectedAction,canvas)) { lastError_=FarmRuntimeHudError::SetupFailed; return false; }
    SoftwareRenderer candidate=renderer; if (!canvas.Draw(router_,candidate)) { lastError_=FarmRuntimeHudError::DrawFailed; return false; } renderer=std::move(candidate); lastError_=FarmRuntimeHudError::None; return true;
}
bool FarmRuntimeHud::RoutePointer(float x,float y,UiPointerPhase phase,FarmPlayerInputBridge& bridge,FarmActionPanelReceipt& receipt) { if(!configured_||!interactive_){lastError_=FarmRuntimeHudError::InputUnavailable;return false;}if(!actionPanel_.RoutePointer(router_,x,y,phase,bridge,receipt)){lastError_=FarmRuntimeHudError::InputRejected;return false;}lastError_=FarmRuntimeHudError::None;return true; }
bool FarmRuntimeHud::RouteKeyboard(UiKeyboardKey key,FarmPlayerInputBridge& bridge,FarmActionPanelReceipt& receipt) { if(!configured_||!interactive_){lastError_=FarmRuntimeHudError::InputUnavailable;return false;}if(!actionPanel_.RouteKeyboard(router_,key,bridge,receipt)){lastError_=FarmRuntimeHudError::InputRejected;return false;}lastError_=FarmRuntimeHudError::None;return true; }
} // namespace NeoEngine
