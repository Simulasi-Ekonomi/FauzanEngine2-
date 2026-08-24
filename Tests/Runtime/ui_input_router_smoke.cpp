#include "Runtime/UiInputRouter.h"

#include <cstdio>

int main() {
    using namespace NeoEngine; UiInputRouter router;
    if(!router.AddWidget({1,0,{0,0,100,100},0,false,true})||!router.AddWidget({2,1,{10,10,50,30},1,true,true})||!router.AddWidget({3,1,{10,10,50,30},2,true,true})||!router.AddWidget({4,1,{10,50,30,20},1,true,true,true})||!router.AddWidget({5,1,{50,50,30,20},1,true,true,true})||router.HitTest(20,20)!=3) return 1;
    const UiPointerResult press=router.RoutePointer(20,20,UiPointerPhase::Press), move=router.RoutePointer(90,90,UiPointerPhase::Move), release=router.RoutePointer(90,90,UiPointerPhase::Release);
    if(press.targetId!=3||!press.captured||move.targetId!=3||!move.captured||release.targetId!=3||!release.captured||router.CapturedWidget()!=0||router.RoutePointer(1,1,UiPointerPhase::Press).consumed) return 1;
    const UiKeyboardResult firstTab=router.RouteKeyboard(UiKeyboardKey::TabForward),secondTab=router.RouteKeyboard(UiKeyboardKey::TabForward),backTab=router.RouteKeyboard(UiKeyboardKey::TabBackward),activate=router.RouteKeyboard(UiKeyboardKey::Activate);
    if(firstTab.targetId!=4||!firstTab.consumed||secondTab.targetId!=5||backTab.targetId!=4||activate.targetId!=4||!activate.activated||router.FocusedWidget()!=4||router.RoutePointer(20,60,UiPointerPhase::Press).targetId!=4||router.FocusedWidget()!=4) return 1;
    if(router.SetFocus(3)||router.LastError()!=UiError::FocusUnavailable||router.RouteKeyboard(static_cast<UiKeyboardKey>(99)).consumed||router.LastError()!=UiError::InvalidKey||!router.RemoveWidget(4)||router.FocusedWidget()!=0||router.RouteKeyboard(UiKeyboardKey::TabBackward).targetId!=5||!router.RouteKeyboard(UiKeyboardKey::ClearFocus).consumed||router.FocusedWidget()!=0) return 1;
    if(router.AddWidget({3,0,{0,0,1,1},0,true,true})||router.LastError()!=UiError::DuplicateWidget||router.AddWidget({4,99,{0,0,1,1},0,true,true})||router.LastError()!=UiError::MissingParent||router.AddWidget({4,0,{0,0,0,1},0,true,true})||router.LastError()!=UiError::InvalidRect||router.RemoveWidget(1)||router.LastError()!=UiError::MissingParent) return 1;
    std::printf("UI_INPUT_ROUTER_SMOKE_OK hit=3 capture=1 focus=1 keyboard=1 release=1 validation=1\n"); return 0;
}
