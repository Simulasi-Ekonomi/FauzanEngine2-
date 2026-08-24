#include "Runtime/GridRouteFollower.h"

#include <cmath>
#include <cstdio>

int main(){
    using namespace NeoEngine;
    SceneWorld world;SceneEntity entity{};if(!world.Create(entity)||!world.SetTransform(entity,{0,0,0,0,0,0,1,1,1}))return 1;
    KinematicMotionController controller;if(!controller.Initialize({2.0F,0.25F}))return 1;
    GridNavigation navigation;if(!navigation.Initialize(4U))return 1;
    GridRouteFollower follower;const GridCell route[]{{0,0},{1,0},{1,1}};
    if(!follower.SetRoute(route)||!follower.Step(world,entity,controller,navigation,1.0F)||!follower.ReachedGoal())return 1;
    const Transform3* arrived=world.GetLocalTransform(entity);if(arrived==nullptr||std::fabs(arrived->x-1.0F)>0.0001F||std::fabs(arrived->z-1.0F)>0.0001F)return 1;
    GridRouteFollower invalid;const GridCell broken[]{{0,0},{2,0}};if(invalid.SetRoute(broken)||invalid.LastError()!=GridRouteFollowerError::InvalidRoute)return 1;
    GridRouteFollower mismatch;if(!mismatch.SetRoute(route)||mismatch.Step(world,entity,controller,navigation,0.1F)||mismatch.LastError()!=GridRouteFollowerError::StartMismatch||world.GetLocalTransform(entity)->x!=arrived->x)return 1;
    SceneWorld blockedWorld;SceneEntity blockedEntity{};if(!blockedWorld.Create(blockedEntity)||!blockedWorld.SetTransform(blockedEntity,{0,0,0,0,0,0,1,1,1}))return 1;
    GridRouteFollower blocked;if(!blocked.SetRoute(route)||!navigation.SetBlocked({1,0},true)||blocked.Step(blockedWorld,blockedEntity,controller,navigation,0.25F)||blocked.LastError()!=GridRouteFollowerError::BlockedTarget||blockedWorld.GetLocalTransform(blockedEntity)->x!=0.0F)return 1;
    if(!blocked.Replan(blockedWorld,blockedEntity,navigation)||!blocked.Step(blockedWorld,blockedEntity,controller,navigation,1.0F)||!blocked.ReachedGoal()||blockedWorld.GetLocalTransform(blockedEntity)->x!=1.0F||blockedWorld.GetLocalTransform(blockedEntity)->z!=1.0F)return 1;
    SceneWorld lateBlockedWorld;SceneEntity lateBlockedEntity{};if(!lateBlockedWorld.Create(lateBlockedEntity)||!lateBlockedWorld.SetTransform(lateBlockedEntity,{0,0,0,0,0,0,1,1,1}))return 1;
    GridNavigation lateNavigation;if(!lateNavigation.Initialize(4U))return 1;const GridCell lateRoute[]{{0,0},{1,0},{2,0}};GridRouteFollower lateBlocked;
    if(!lateBlocked.SetRoute(lateRoute)||!lateNavigation.SetBlocked({2,0},true)||lateBlocked.Step(lateBlockedWorld,lateBlockedEntity,controller,lateNavigation,1.0F)||lateBlocked.LastError()!=GridRouteFollowerError::BlockedTarget||lateBlockedWorld.GetLocalTransform(lateBlockedEntity)->x!=0.0F||lateBlocked.Replan(lateBlockedWorld,lateBlockedEntity,lateNavigation)||lateBlocked.LastError()!=GridRouteFollowerError::ReplanFailed)return 1;
    if(!lateNavigation.SetBlocked({2,0},false)||!lateBlocked.Step(lateBlockedWorld,lateBlockedEntity,controller,lateNavigation,1.0F)||!lateBlocked.ReachedGoal()||lateBlockedWorld.GetLocalTransform(lateBlockedEntity)->x!=2.0F)return 1;
    std::printf("GRID_ROUTE_FOLLOWER_SMOKE_OK arrival=1 chunks=4 obstacle=1 atomic_late_obstacle=1 replan=1 deterministic=1 x=%.1f z=%.1f\n",arrived->x,arrived->z);
}
