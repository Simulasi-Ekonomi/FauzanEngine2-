#include "Runtime/GridRouteFollower.h"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace NeoEngine;

int main(){
    GridNavigation navigation;if(!navigation.Initialize(4U))return 1;
    SceneWorld world;SceneEntity entity{};if(!world.Create(entity)||!world.SetTransform(entity,{0.0F,0.0F,0.0F,0,0,0,1,1,1}))return 1;
    GridRouteFollower follower;const std::vector<GridCell> route{{0U,0U},{1U,0U},{1U,1U}};if(!follower.SetRoute(route))return 1;
    RouteIntent intent{};if(!follower.PeekIntent(world,entity,navigation,intent)||intent.entity!=entity||intent.from!=GridCell{0U,0U}||intent.target!=GridCell{1U,0U}||std::fabs(intent.remainingPlanarDistance-1.0F)>0.0001F)return 1;
    const Transform3* initial=world.GetLocalTransform(entity);if(initial==nullptr||initial->x!=0.0F||initial->z!=0.0F||follower.ReachedGoal())return 1;
    if(follower.CommitIntent(world,navigation,{intent,false})||follower.LastError()!=GridRouteFollowerError::IntentReceiptInvalid)return 1;
    if(!world.SetTransform(entity,{0.5F,0.0F,0.0F,0,0,0,1,1,1})||!follower.CommitIntent(world,navigation,{intent,true})||follower.ReachedGoal())return 1;
    RouteIntent partial{};if(!follower.PeekIntent(world,entity,navigation,partial)||partial.routeIndex!=intent.routeIndex||std::fabs(partial.remainingPlanarDistance-0.5F)>0.0001F)return 1;
    RouteIntent preserved=partial;if(!navigation.SetBlocked({1U,0U},true)||follower.PeekIntent(world,entity,navigation,partial)||follower.LastError()!=GridRouteFollowerError::BlockedTarget||partial.target!=preserved.target||!navigation.SetBlocked({1U,0U},false))return 1;
    if(!world.SetTransform(entity,{1.0F,0.0F,0.0F,0,0,0,1,1,1})||!follower.CommitIntent(world,navigation,{intent,true}))return 1;
    if(follower.CommitIntent(world,navigation,{intent,true})||follower.LastError()!=GridRouteFollowerError::IntentStale)return 1;
    RouteIntent next{};if(!follower.PeekIntent(world,entity,navigation,next)||next.from!=GridCell{1U,0U}||next.target!=GridCell{1U,1U})return 1;
    RouteIntent malformed=next;malformed.targetX=2.0F;if(follower.CommitIntent(world,navigation,{malformed,true})||follower.LastError()!=GridRouteFollowerError::IntentStale)return 1;
    if(!world.SetTransform(entity,{1.0F,0.0F,1.0F,0,0,0,1,1,1})||!follower.CommitIntent(world,navigation,{next,true})||!follower.ReachedGoal())return 1;
    const Transform3* finished=world.GetLocalTransform(entity);if(finished==nullptr||finished->x!=1.0F||finished->z!=1.0F){return 1;}
    std::printf("GRID_ROUTE_INTENT_SMOKE_OK peek=1 partial=1 blocked=1 stale=1 arrival=1\n");return 0;
}
