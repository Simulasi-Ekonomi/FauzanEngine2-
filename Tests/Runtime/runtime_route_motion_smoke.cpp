#include "Runtime/NeoRuntime.h"

#include <cmath>
#include <cstdio>

int main(){
    using namespace NeoEngine;
    NeoRuntime baseline;if(!baseline.Initialize({})||baseline.RouteNavigation()!=nullptr||baseline.RouteMotionEntity()!=nullptr||!baseline.Shutdown())return 1;
    NeoRuntime invalid;RuntimeConfig invalidConfig{};invalidConfig.enableRouteMotion=true;invalidConfig.routeMotionRoute={{0,0}};
    if(invalid.Initialize(invalidConfig)||invalid.LastError()!=RuntimeError::InvalidConfiguration)return 1;
    NeoRuntime runtime;RuntimeConfig config{};config.enableRouteMotion=true;config.routeMotionUnitsPerSecond=30.0F;config.routeMotionFaceMovementDirection=true;config.routeMotionNavigationSide=4;config.routeMotionRoute={{0,0},{1,0}};
    if(!runtime.Initialize(config)||runtime.RouteNavigation()==nullptr||runtime.RouteMotionEntity()==nullptr)return 1;
    const SceneEntity entity=*runtime.RouteMotionEntity();if(runtime.MotionAuthority()==nullptr||!runtime.Tick()||runtime.MotionAuthority()->Acquire(entity,MovementAuthority::SkeletalRoot)||runtime.MotionAuthority()->LastError()!=MovementAuthorityError::Conflict)return 1;
    const Transform3* halfway=runtime.Scene()->GetLocalTransform(entity);if(halfway==nullptr||std::fabs(halfway->x-0.5F)>0.0001F||std::fabs(halfway->z)>0.0001F||std::fabs(halfway->ry-1.5707963F)>0.0001F)return 1;
    const Transform3 pausedPreserved=*halfway;if(!runtime.SetPaused(true)||!runtime.Tick())return 1;
    const Transform3* paused=runtime.Scene()->GetLocalTransform(entity);if(paused==nullptr||paused->x!=pausedPreserved.x||paused->z!=pausedPreserved.z||!runtime.SetPaused(false)||!runtime.Tick())return 1;
    const Transform3* arrived=runtime.Scene()->GetLocalTransform(entity);if(arrived==nullptr||std::fabs(arrived->x-1.0F)>0.0001F||std::fabs(arrived->z)>0.0001F||runtime.MotionAuthority()->Acquire(entity,MovementAuthority::SkeletalRoot)||runtime.MotionAuthority()->LastError()!=MovementAuthorityError::Conflict)return 1;
    const Transform3 preserved=*arrived;if(!runtime.Tick())return 1;
    const Transform3* stable=runtime.Scene()->GetLocalTransform(entity);if(stable==nullptr||stable->x!=preserved.x||stable->z!=preserved.z||!runtime.MotionAuthority()->Acquire(entity,MovementAuthority::SkeletalRoot))return 1;
    if(!runtime.Shutdown()||runtime.RouteNavigation()!=nullptr||runtime.RouteMotionEntity()!=nullptr)return 1;
    NeoRuntime replanning;RuntimeConfig replanConfig{};replanConfig.enableRouteMotion=true;replanConfig.routeMotionUnitsPerSecond=60.0F;replanConfig.routeMotionNavigationSide=4;replanConfig.routeMotionRoute={{0,0},{1,0},{2,0},{3,0}};
    if(!replanning.Initialize(replanConfig)||replanning.ReplanRouteMotion()||replanning.LastError()!=RuntimeError::RouteReplanFailed||!replanning.Tick()||!replanning.RouteNavigation()->SetBlocked({2,0},true)||!replanning.ReplanRouteMotion())return 1;
    for(int tick=0;tick<4;++tick)if(!replanning.Tick())return 1;
    const Transform3* detoured=replanning.Scene()->GetLocalTransform(*replanning.RouteMotionEntity());if(detoured==nullptr||std::fabs(detoured->x-3.0F)>0.0001F||std::fabs(detoured->z)>0.0001F||!replanning.Shutdown())return 1;
    NeoRuntime blocked;RuntimeConfig blockedConfig{};blockedConfig.enableRouteMotion=true;blockedConfig.routeMotionUnitsPerSecond=60.0F;blockedConfig.routeMotionNavigationSide=4;blockedConfig.routeMotionRoute={{0,0},{1,0}};
    if(!blocked.Initialize(blockedConfig)||blocked.RouteNavigation()==nullptr||!blocked.RouteNavigation()->SetBlocked({1,0},true)||blocked.Tick()||blocked.LastError()!=RuntimeError::RouteMotionFailed)return 1;
    const SceneEntity blockedEntity=*blocked.RouteMotionEntity();const Transform3* unchanged=blocked.Scene()->GetLocalTransform(blockedEntity);if(unchanged==nullptr||unchanged->x!=0.0F||unchanged->z!=0.0F||!blocked.Shutdown())return 1;
    std::printf("RUNTIME_ROUTE_MOTION_SMOKE_OK fixedTick=1 arrival=1 pause=1 manualReplan=1 blocked=1 cleanup=1\n");
}
