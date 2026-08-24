#include "Runtime/InputMotionBridge.h"

#include <cmath>
#include <cstdio>

int main(){
    using namespace NeoEngine;
    InputState input;const int32_t forward=MakeInputCode(InputDeviceType::Keyboard,1),backward=MakeInputCode(InputDeviceType::Keyboard,2),left=MakeInputCode(InputDeviceType::Keyboard,3),right=MakeInputCode(InputDeviceType::Keyboard,4);
    if(!input.Bind("move_forward",forward)||!input.Bind("move_backward",backward)||!input.Bind("move_left",left)||!input.Bind("move_right",right))return 1;
    SceneWorld world;SceneEntity entity{};if(!world.Create(entity)||!world.SetTransform(entity,{0,0,0,0,0,0,1,1,1}))return 1;
    KinematicMotionController controller;if(!controller.Initialize({5.0F,0.25F}))return 1;
    InputMotionBridge bridge;if(bridge.Initialize({"same","same","left","right"})||bridge.LastError()!=InputMotionBridgeError::InvalidConfiguration||!bridge.Initialize())return 1;
    if(!input.Push(forward,true)||!input.Push(right,true)){return 1;}input.BeginFrame();if(!bridge.Step(input,controller,world,entity,0.2F))return 1;
    const Transform3* moved=world.GetLocalTransform(entity);if(moved==nullptr||std::fabs(moved->x-0.7071067F)>0.0002F||std::fabs(moved->z-0.7071067F)>0.0002F)return 1;
    if(!input.Push(forward,false)||!input.Push(right,false)){return 1;}input.BeginFrame();if(!bridge.Step(input,controller,world,entity,0.2F)||world.GetLocalTransform(entity)->x!=moved->x||world.GetLocalTransform(entity)->z!=moved->z)return 1;
    const Transform3 preserved=*world.GetLocalTransform(entity);InputMotionBridge missing;if(!missing.Initialize({"unknown","move_backward","move_left","move_right"})||missing.Step(input,controller,world,entity,0.2F)||missing.LastError()!=InputMotionBridgeError::MissingAction||world.GetLocalTransform(entity)->x!=preserved.x)return 1;
    std::printf("INPUT_MOTION_BRIDGE_SMOKE_OK diagonal=1 idle=1 missing=1 x=%.6f z=%.6f\n",preserved.x,preserved.z);
}
