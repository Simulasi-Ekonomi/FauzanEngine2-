#include "Runtime/AnimationLocomotionBridge.h"

#include <cmath>
#include <cstdio>
#include <limits>

int main() {
    using namespace NeoEngine;
    AnimationLocomotionBridge bridge; AnimationStateMachine machine;
    if(bridge.Initialize({"","back",0.1F})||bridge.LastError()!=AnimationLocomotionBridgeError::InvalidConfiguration||!machine.AddState({"idle","idle"})||!machine.AddState({"locomotion","move"})||!machine.AddTransition({"to_move","idle","locomotion",0.1F})||!machine.AddTransition({"to_idle","locomotion","idle",0.1F})||!bridge.Initialize({"to_move","to_idle",0.1F})||bridge.Apply({0,0},machine)||bridge.LastError()!=AnimationLocomotionBridgeError::StateMachineNotStarted)return 1;
    if(!machine.Start("idle")||!bridge.Apply({0.05F,0.05F},machine)||bridge.IsLocomoting()||machine.IsBlending()||!bridge.Apply({1,0},machine)||!bridge.IsLocomoting()||!machine.IsBlending()||!machine.Update(0.1F)||machine.ActiveStateId()!="locomotion")return 1;
    if(!bridge.Apply({0,0},machine)||bridge.IsLocomoting()||!machine.IsBlending()||!machine.Update(0.1F)||machine.ActiveStateId()!="idle")return 1;
    if(bridge.Apply({std::numeric_limits<float>::quiet_NaN(),0},machine)||bridge.LastError()!=AnimationLocomotionBridgeError::InvalidInput||bridge.IsLocomoting()||machine.ActiveStateId()!="idle")return 1;
    std::printf("ANIMATION_LOCOMOTION_BRIDGE_SMOKE_OK idle=1 locomotion=1 noTransformWrite=1 invalid=1\n"); return 0;
}
