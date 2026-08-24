#include "Runtime/NeoRuntime.h"

#include <cmath>
#include <cstdio>

int main(){
    using namespace NeoEngine;
    NeoRuntime invalid;RuntimeConfig invalidConfig{};invalidConfig.enableInputMotion=true;invalidConfig.inputMotionUnitsPerSecond=0.0F;
    if(invalid.Initialize(invalidConfig)||invalid.LastError()!=RuntimeError::InvalidConfiguration)return 1;
    NeoRuntime runtime;RuntimeConfig config{};config.enableInputMotion=true;config.inputMotionUnitsPerSecond=60.0F;
    if(!runtime.Initialize(config)||runtime.Input()==nullptr||runtime.InputMotionEntity()==nullptr)return 1;
    InputState* input=runtime.Input();const SceneEntity entity=*runtime.InputMotionEntity();
    if(!input->Push(MakeInputCode(InputDeviceType::Keyboard,1U),true)||!input->Push(MakeInputCode(InputDeviceType::Keyboard,4U),true)||!runtime.Tick())return 1;
    const Transform3* moved=runtime.Scene()->GetLocalTransform(entity);if(moved==nullptr||std::fabs(moved->x-0.7071067F)>0.0002F||std::fabs(moved->z-0.7071067F)>0.0002F||moved->ry!=0.0F)return 1;
    const Transform3 preserved=*moved;
    if(!runtime.SetPaused(true)||!runtime.Tick())return 1;
    const Transform3* paused=runtime.Scene()->GetLocalTransform(entity);if(paused==nullptr||paused->x!=preserved.x||paused->z!=preserved.z)return 1;
    if(!runtime.Shutdown()||runtime.Input()!=nullptr||runtime.InputMotionEntity()!=nullptr)return 1;
    NeoRuntime headingRuntime;RuntimeConfig headingConfig{};headingConfig.enableInputMotion=true;headingConfig.inputMotionUnitsPerSecond=60.0F;headingConfig.inputMotionFaceMovementDirection=true;
    if(!headingRuntime.Initialize(headingConfig)||headingRuntime.Input()==nullptr||!headingRuntime.Input()->Push(MakeInputCode(InputDeviceType::Keyboard,1U),true)||!headingRuntime.Input()->Push(MakeInputCode(InputDeviceType::Keyboard,4U),true)||!headingRuntime.Tick())return 1;
    const Transform3* headed=headingRuntime.Scene()->GetLocalTransform(*headingRuntime.InputMotionEntity());if(headed==nullptr||std::fabs(headed->ry-0.78539816F)>0.0002F||!headingRuntime.Shutdown())return 1;
    std::printf("RUNTIME_INPUT_MOTION_SMOKE_OK fixedTick=1 diagonal=1 heading=1 pause=1 x=%.6f z=%.6f\n",preserved.x,preserved.z);
}
