#include "Runtime/InputState.h"
#include <cstdio>

using namespace NeoEngine;

int main(){
    InputState i;
    const int32_t keyboard=MakeInputCode(InputDeviceType::Keyboard,32),touch=MakeInputCode(InputDeviceType::Touch,7),gamepad=MakeInputCode(InputDeviceType::Gamepad,1);
    bool ok=i.Summary().boundActions==0U&&i.Bind("farm.harvest",keyboard)&&i.Bind("farm.tap",touch)&&!i.Bind("farm.harvest",keyboard)&&i.HasAction("farm.tap")&&i.Push(keyboard,true);
    const InputStateSummary queued=i.Summary();
    ok=ok&&queued.boundActions==2U&&queued.pressedActions==0U&&queued.pendingEvents==1U;
    i.BeginFrame();
    auto a=i.Query("farm.harvest");
    const InputStateSummary pressed=i.Summary();
    ok=ok&&a.pressed&&a.justPressed&&!a.justReleased&&pressed.pressedActions==1U&&pressed.justPressedActions==1U&&pressed.justReleasedActions==0U&&pressed.pendingEvents==0U&&i.Push(keyboard,false);
    i.BeginFrame();
    a=i.Query("farm.harvest");
    const InputStateSummary released=i.Summary();
    ok=ok&&!a.pressed&&!a.justPressed&&a.justReleased&&released.pressedActions==0U&&released.justReleasedActions==1U&&i.Rebind("farm.harvest",gamepad)&&i.Push(gamepad,true);
    i.BeginFrame();
    a=i.Query("farm.harvest");
    const InputStateSummary rebound=i.Summary();
    ok=ok&&a.pressed&&a.justPressed&&rebound.boundActions==2U&&rebound.pressedActions==1U&&rebound.justPressedActions==1U&&rebound.pendingEvents==0U&&!i.Rebind("missing",gamepad)&&i.LastError()==InputError::MissingAction;
    if(!ok)return 1;
    std::printf("INPUT_STATE_SMOKE_OK keyboard=1 touch=1 gamepad=1 rebind=1 summary=1 deterministic=1\n");
}
