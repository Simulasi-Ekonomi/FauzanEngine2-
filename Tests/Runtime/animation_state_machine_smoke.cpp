#include "Runtime/AnimationStateMachine.h"

#include <cmath>
#include <cstdio>

int main() {
    using namespace NeoEngine;
    AnimationTimeline timeline; if(!timeline.AddTrack("idle",{{0,0},{1,0}})||!timeline.AddTrack("walk",{{0,10},{1,20}}))return 1;
    AnimationStateMachine machine; float value=0; if(machine.AddState({"","idle"})||machine.LastError()!=AnimationStateMachineError::InvalidState||!machine.AddState({"idle","idle",AnimationPlayback::Loop})||!machine.AddState({"walk","walk",AnimationPlayback::Loop})||machine.AddState({"walk","walk"})||machine.LastError()!=AnimationStateMachineError::DuplicateState)return 1;
    if(!machine.AddTransition({"idle_to_walk","idle","walk",0.5F})||!machine.AddTransition({"walk_to_idle","walk","idle",0.0F})||machine.AddTransition({"idle_to_walk","idle","walk",0.5F})||machine.LastError()!=AnimationStateMachineError::DuplicateTransition||machine.Trigger("idle_to_walk")||machine.LastError()!=AnimationStateMachineError::NotStarted)return 1;
    if(!machine.Start("idle")||!machine.Update(0.25F)||!machine.Sample(timeline,value)||std::fabs(value)>0.0001F||!machine.Trigger("idle_to_walk")||!machine.IsBlending()||!machine.Sample(timeline,value)||std::fabs(value)>0.0001F)return 1;
    if(!machine.Update(0.25F)||!machine.Sample(timeline,value)||std::fabs(value-6.25F)>0.0001F||machine.Trigger("idle_to_walk")||machine.LastError()!=AnimationStateMachineError::TransitionInProgress)return 1;
    if(!machine.Update(0.25F)||machine.IsBlending()||machine.ActiveStateId()!="walk"||!machine.Sample(timeline,value)||std::fabs(value-15.0F)>0.0001F||machine.Update(-0.1F)||machine.LastError()!=AnimationStateMachineError::InvalidDelta||!machine.Sample(timeline,value)||std::fabs(value-15.0F)>0.0001F)return 1;
    if(!machine.Trigger("walk_to_idle")||machine.ActiveStateId()!="idle"||machine.IsBlending()||!machine.Sample(timeline,value)||std::fabs(value)>0.0001F)return 1;
    AnimationStateMachine missing; if(!missing.AddState({"missing","not-present"})||!missing.Start("missing")||missing.Sample(timeline,value)||missing.LastError()!=AnimationStateMachineError::SampleFailed)return 1;
    std::printf("ANIMATION_STATE_MACHINE_SMOKE_OK states=2 blend=1 immediate=1 invalid=1 value=%.2f\n",value); return 0;
}
