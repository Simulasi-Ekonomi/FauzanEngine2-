#include "Runtime/AnimationStateMachine.h"

#include <cmath>
#include <cstdio>

int main() {
    using namespace NeoEngine;
    AnimationTimeline timeline; if(!timeline.AddTrack("idle",{{0,0},{1,0}})||!timeline.AddTrack("walk",{{0,10},{1,20}})||!timeline.AddEventMarker("idle",{"idle_transition_notify",0.2F})||!timeline.AddEventMarker("walk",{"walk_transition_notify",0.1F}))return 1;
    AnimationStateMachine machine; AnimationStateMachineSnapshot snapshot{}; float value=0; if(machine.AddState({"","idle"})||machine.LastError()!=AnimationStateMachineError::InvalidState||!machine.AddState({"idle","idle",AnimationPlayback::Loop})||!machine.AddState({"walk","walk",AnimationPlayback::Loop})||machine.AddState({"walk","walk"})||machine.LastError()!=AnimationStateMachineError::DuplicateState)return 1;
    if(!machine.AddTransition({"idle_to_walk","idle","walk",0.5F})||!machine.AddTransition({"walk_to_idle","walk","idle",0.0F})||machine.AddTransition({"idle_to_walk","idle","walk",0.5F})||machine.LastError()!=AnimationStateMachineError::DuplicateTransition||machine.Trigger("idle_to_walk")||machine.LastError()!=AnimationStateMachineError::NotStarted)return 1;
    if(!machine.Start("idle")||!machine.Snapshot(snapshot)||snapshot.activeStateId!="idle"||snapshot.blending||snapshot.activeTimeSeconds!=0.0F||!machine.Update(0.25F)||!machine.Sample(timeline,value)||std::fabs(value)>0.0001F||!machine.Trigger("idle_to_walk")||!machine.IsBlending()||!machine.Snapshot(snapshot)||snapshot.targetStateId!="walk"||snapshot.transitionId!="idle_to_walk"||!snapshot.blending||std::fabs(snapshot.activeTimeSeconds-0.25F)>0.0001F||snapshot.targetTimeSeconds!=0.0F||snapshot.blendFraction!=0.0F||!machine.Sample(timeline,value)||std::fabs(value)>0.0001F)return 1;
    if(!machine.Update(0.25F)||!machine.Snapshot(snapshot)||snapshot.transitionId!="idle_to_walk"||std::fabs(snapshot.blendFraction-0.5F)>0.0001F||std::fabs(snapshot.targetTimeSeconds-0.25F)>0.0001F||!machine.Sample(timeline,value)||std::fabs(value-6.25F)>0.0001F||!([&] { AnimationStateMachineSnapshot invalid = snapshot; invalid.transitionId = "walk_to_idle"; return !machine.Restore(invalid) && machine.LastError() == AnimationStateMachineError::InvalidSnapshot && machine.Restore(snapshot); })()||machine.Trigger("idle_to_walk")||machine.LastError()!=AnimationStateMachineError::TransitionInProgress)return 1;
    std::vector<std::string> transitionEvents;
    if(!machine.CollectEvents(timeline,0.0F,0.25F,transitionEvents)||transitionEvents.size()!=2U||transitionEvents[0]!="idle_transition_notify"||transitionEvents[1]!="walk_transition_notify")return 1;
    transitionEvents={"preserved"};
    if(machine.CollectEvents(timeline,0.5F,0.25F,transitionEvents)||machine.LastError()!=AnimationStateMachineError::EventCollectionFailed||transitionEvents.size()!=1U||transitionEvents[0]!="preserved")return 1;
    if(!machine.Update(0.25F)||machine.IsBlending()||machine.ActiveStateId()!="walk"||!machine.Snapshot(snapshot)||snapshot.blending||!snapshot.targetStateId.empty()||std::fabs(snapshot.activeTimeSeconds-0.5F)>0.0001F||!machine.Sample(timeline,value)||std::fabs(value-15.0F)>0.0001F||machine.Update(-0.1F)||machine.LastError()!=AnimationStateMachineError::InvalidDelta||!machine.Sample(timeline,value)||std::fabs(value-15.0F)>0.0001F||machine.Update(1.1F)||machine.LastError()!=AnimationStateMachineError::InvalidDelta||!machine.Sample(timeline,value)||std::fabs(value-15.0F)>0.0001F)return 1;
    AnimationStateMachineSnapshot residualTargetTime = snapshot;
    residualTargetTime.targetTimeSeconds = 1.0F;
    if(machine.Restore(residualTargetTime)||machine.LastError()!=AnimationStateMachineError::InvalidSnapshot||machine.ActiveStateId()!="walk"||machine.IsBlending())return 1;
    if(!machine.Trigger("walk_to_idle")||machine.ActiveStateId()!="idle"||machine.IsBlending()||!machine.Sample(timeline,value)||std::fabs(value)>0.0001F)return 1;
    AnimationStateMachine missing; if(!missing.AddState({"missing","not-present"})||missing.Snapshot(snapshot)||missing.LastError()!=AnimationStateMachineError::NotStarted||!missing.Start("missing")||missing.Sample(timeline,value)||missing.LastError()!=AnimationStateMachineError::SampleFailed)return 1;
    std::printf("ANIMATION_STATE_MACHINE_SMOKE_OK states=2 blend=1 snapshot=1 immediate=1 invalid=1 value=%.2f\n",value); return 0;
}
