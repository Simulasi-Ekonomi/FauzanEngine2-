#include "Runtime/KinematicMotionController.h"

#include <cmath>
#include <cstdio>
#include <limits>

int main(){
    using namespace NeoEngine;
    SceneWorld world;SceneEntity entity{};
    if(!world.Create(entity)||!world.SetTransform(entity,{1.0F,2.0F,3.0F,0,0,0,1,1,1}))return 1;
    KinematicMotionController controller;
    if(controller.Initialize({0.0F,0.1F})||controller.LastError()!=KinematicMotionError::InvalidConfiguration||!controller.Initialize({10.0F,0.25F}))return 1;
    if(!controller.Step(world,entity,{3.0F,4.0F},0.1F))return 1;
    const Transform3* moved=world.GetLocalTransform(entity);if(moved==nullptr||std::fabs(moved->x-1.6F)>0.0001F||std::fabs(moved->z-3.8F)>0.0001F)return 1;
    if(!controller.Step(world,entity,{0.0F,0.0F},0.1F)||world.GetLocalTransform(entity)->x!=moved->x||world.GetLocalTransform(entity)->z!=moved->z)return 1;
    const Transform3 preserved=*world.GetLocalTransform(entity);
    if(controller.Step(world,entity,{std::numeric_limits<float>::infinity(),0.0F},0.1F)||controller.LastError()!=KinematicMotionError::InvalidInput||world.GetLocalTransform(entity)->x!=preserved.x||world.GetLocalTransform(entity)->z!=preserved.z)return 1;
    if(controller.Step(world,entity,{1.0F,0.0F},0.5F)||controller.LastError()!=KinematicMotionError::InvalidInput||controller.Step(world,{}, {1.0F,0.0F},0.1F)||controller.LastError()!=KinematicMotionError::MissingEntity)return 1;
    KinematicMotionController heading;if(!heading.Initialize({10.0F,0.25F,true})||!heading.Step(world,entity,{1.0F,0.0F},0.1F)||std::fabs(world.GetLocalTransform(entity)->ry-1.5707963F)>0.0001F)return 1;
    if(!heading.Step(world,entity,{3.0F,4.0F},0.1F)||std::fabs(world.GetLocalTransform(entity)->ry-std::atan2(3.0F,4.0F))>0.0001F)return 1;
    const Transform3 headingPreserved=*world.GetLocalTransform(entity);if(heading.Step(world,entity,{std::numeric_limits<float>::infinity(),0.0F},0.1F)||world.GetLocalTransform(entity)->ry!=headingPreserved.ry)return 1;
    const float headingYaw=world.GetLocalTransform(entity)->ry;if(!heading.Step(world,entity,{0.0F,0.0F},0.1F)||world.GetLocalTransform(entity)->ry!=headingYaw)return 1;
    std::printf("KINEMATIC_MOTION_CONTROLLER_SMOKE_OK normalized=1 heading=1 bounded=1 atomic=1 x=%.1f z=%.1f\n",preserved.x,preserved.z);
}
