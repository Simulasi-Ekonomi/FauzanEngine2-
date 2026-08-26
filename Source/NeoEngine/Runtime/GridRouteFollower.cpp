#include "GridRouteFollower.h"

#include <algorithm>
#include <cmath>

namespace NeoEngine {
bool GridRouteFollower::SetRoute(std::span<const GridCell> route){
    if(route.size()<2U||route.size()>kMaxRouteCells){lastError_=GridRouteFollowerError::InvalidRoute;return false;}
    for(size_t index=1U;index<route.size();++index){const int32_t dx=static_cast<int32_t>(route[index].x)-route[index-1U].x,dz=static_cast<int32_t>(route[index].z)-route[index-1U].z;if(std::abs(dx)+std::abs(dz)!=1){lastError_=GridRouteFollowerError::InvalidRoute;return false;}}
    route_.assign(route.begin(),route.end());routeIndex_=1U;++routeRevision_;if(routeRevision_==0U)++routeRevision_;started_=false;lastError_=GridRouteFollowerError::None;return true;
}
bool GridRouteFollower::Step(SceneWorld& world,SceneEntity entity,KinematicMotionController& controller,const GridNavigation& navigation,float seconds){
    GridRouteFollower candidateFollower=*this;
    SceneWorld candidateWorld=world;
    KinematicMotionController candidateController=controller;
    if(!candidateFollower.StepInPlace(candidateWorld,entity,candidateController,navigation,seconds)){lastError_=candidateFollower.lastError_;return false;}
    world=std::move(candidateWorld);controller=std::move(candidateController);*this=std::move(candidateFollower);return true;
}
bool GridRouteFollower::StepGuarded(SceneWorld& world,SceneEntity entity,KinematicMotionController& controller,const GridNavigation& navigation,float seconds,MovementAuthorityGate& authority){
    MovementAuthorityGate candidateAuthority=authority;
    if(!candidateAuthority.Acquire(entity,MovementAuthority::KinematicRoute)){lastError_=GridRouteFollowerError::AuthorityConflict;return false;}
    if(!Step(world,entity,controller,navigation,seconds))return false;
    authority=std::move(candidateAuthority);return true;
}
bool GridRouteFollower::PeekIntent(const SceneWorld& world,SceneEntity entity,const GridNavigation& navigation,RouteIntent& out){
    if(route_.empty()||routeIndex_==0U||routeIndex_>=route_.size()||routeRevision_==0U){lastError_=GridRouteFollowerError::NotReady;return false;}
    const Transform3* current=world.GetLocalTransform(entity);if(current==nullptr){lastError_=GridRouteFollowerError::MissingEntity;return false;}
    constexpr float epsilon=0.0001F;
    if(!started_&&(std::fabs(current->x-static_cast<float>(route_.front().x))>epsilon||std::fabs(current->z-static_cast<float>(route_.front().z))>epsilon)){lastError_=GridRouteFollowerError::StartMismatch;return false;}
    const GridCell from=route_[routeIndex_-1U],target=route_[routeIndex_];
    if(navigation.IsBlocked(from)||navigation.IsBlocked(target)){lastError_=GridRouteFollowerError::BlockedTarget;return false;}
    const float targetX=static_cast<float>(target.x),targetZ=static_cast<float>(target.z),dx=targetX-current->x,dz=targetZ-current->z,distance=std::sqrt(dx*dx+dz*dz);
    if(!std::isfinite(distance)){lastError_=GridRouteFollowerError::TransformFailed;return false;}
    RouteIntent candidate{};candidate.entity=entity;candidate.routeRevision=routeRevision_;candidate.routeIndex=static_cast<uint16_t>(routeIndex_);candidate.from=from;candidate.target=target;candidate.targetX=targetX;candidate.targetZ=targetZ;candidate.remainingPlanarDistance=distance;
    out=candidate;lastError_=GridRouteFollowerError::None;return true;
}
bool GridRouteFollower::CommitIntent(const SceneWorld& world,const GridNavigation& navigation,const RouteIntentReceipt& receipt){
    if(!receipt.motionApplied){lastError_=GridRouteFollowerError::IntentReceiptInvalid;return false;}
    const RouteIntent& intent=receipt.intent;
    if(route_.empty()||routeIndex_==0U||routeIndex_>=route_.size()||intent.routeRevision==0U||intent.routeRevision!=routeRevision_||intent.routeIndex!=routeIndex_||intent.from!=route_[routeIndex_-1U]||intent.target!=route_[routeIndex_]||intent.targetX!=static_cast<float>(route_[routeIndex_].x)||intent.targetZ!=static_cast<float>(route_[routeIndex_].z)){lastError_=GridRouteFollowerError::IntentStale;return false;}
    if(navigation.IsBlocked(intent.from)||navigation.IsBlocked(intent.target)){lastError_=GridRouteFollowerError::BlockedTarget;return false;}
    const Transform3* current=world.GetLocalTransform(intent.entity);if(current==nullptr){lastError_=GridRouteFollowerError::MissingEntity;return false;}
    constexpr float epsilon=0.0001F;const float fromX=static_cast<float>(intent.from.x),fromZ=static_cast<float>(intent.from.z);const bool horizontal=intent.from.z==intent.target.z;
    const bool onSegment=horizontal?(std::fabs(current->z-fromZ)<=epsilon&&current->x>=std::min(fromX,intent.targetX)-epsilon&&current->x<=std::max(fromX,intent.targetX)+epsilon):(std::fabs(current->x-fromX)<=epsilon&&current->z>=std::min(fromZ,intent.targetZ)-epsilon&&current->z<=std::max(fromZ,intent.targetZ)+epsilon);
    if(!onSegment){lastError_=GridRouteFollowerError::IntentReceiptInvalid;return false;}
    const float dx=intent.targetX-current->x,dz=intent.targetZ-current->z,distance=std::sqrt(dx*dx+dz*dz);if(!std::isfinite(distance)){lastError_=GridRouteFollowerError::IntentReceiptInvalid;return false;}
    started_=true;if(distance<=epsilon)++routeIndex_;lastError_=GridRouteFollowerError::None;return true;
}
bool GridRouteFollower::StepInPlace(SceneWorld& world,SceneEntity entity,KinematicMotionController& controller,const GridNavigation& navigation,float seconds){
    if(route_.empty()||routeIndex_==0U){lastError_=GridRouteFollowerError::NotReady;return false;}
    if(!std::isfinite(seconds)||seconds<0.0F||!controller.IsReady()){lastError_=GridRouteFollowerError::NotReady;return false;}
    const Transform3* initial=world.GetLocalTransform(entity);if(initial==nullptr){lastError_=GridRouteFollowerError::MissingEntity;return false;}
    constexpr float epsilon=0.0001F;
    if(!started_&&(std::fabs(initial->x-static_cast<float>(route_.front().x))>epsilon||std::fabs(initial->z-static_cast<float>(route_.front().z))>epsilon)){lastError_=GridRouteFollowerError::StartMismatch;return false;}
    if(navigation.IsBlocked(route_.front())){lastError_=GridRouteFollowerError::BlockedTarget;return false;}
    started_=true;float remaining=seconds;
    while(remaining>epsilon&&routeIndex_<route_.size()){
        if(navigation.IsBlocked(route_[routeIndex_])){lastError_=GridRouteFollowerError::BlockedTarget;return false;}
        const Transform3* current=world.GetLocalTransform(entity);if(current==nullptr){lastError_=GridRouteFollowerError::MissingEntity;return false;}
        const float targetX=static_cast<float>(route_[routeIndex_].x),targetZ=static_cast<float>(route_[routeIndex_].z),dx=targetX-current->x,dz=targetZ-current->z,distance=std::sqrt(dx*dx+dz*dz);
        if(!std::isfinite(distance)){lastError_=GridRouteFollowerError::TransformFailed;return false;}
        if(distance<=epsilon){++routeIndex_;continue;}
        const float arrivalSeconds=distance/controller.UnitsPerSecond(),stepSeconds=std::min({remaining,arrivalSeconds,controller.MaxStepSeconds()});
        if(!controller.Step(world,entity,{dx,dz},stepSeconds)){lastError_=GridRouteFollowerError::ControllerFailed;return false;}
        remaining-=stepSeconds;
        if(stepSeconds+epsilon>=arrivalSeconds){const Transform3* moved=world.GetLocalTransform(entity);if(moved==nullptr){lastError_=GridRouteFollowerError::MissingEntity;return false;}const float remainingX=targetX-moved->x,remainingZ=targetZ-moved->z;if(!std::isfinite(remainingX)||!std::isfinite(remainingZ)){lastError_=GridRouteFollowerError::TransformFailed;return false;}if(std::sqrt(remainingX*remainingX+remainingZ*remainingZ)<=epsilon)++routeIndex_;}
    }
    lastError_=GridRouteFollowerError::None;return true;
}
bool GridRouteFollower::Replan(SceneWorld& world,SceneEntity entity,const GridNavigation& navigation){
    if(route_.empty()||routeIndex_==0U||routeIndex_>=route_.size()||(!started_&&lastError_!=GridRouteFollowerError::BlockedTarget)){lastError_=GridRouteFollowerError::NotReady;return false;}
    const Transform3* current=world.GetLocalTransform(entity);if(current==nullptr){lastError_=GridRouteFollowerError::MissingEntity;return false;}
    constexpr float epsilon=0.0001F;const GridCell start=started_?route_[routeIndex_-1U]:route_.front();
    if(std::fabs(current->x-static_cast<float>(start.x))>epsilon||std::fabs(current->z-static_cast<float>(start.z))>epsilon){lastError_=GridRouteFollowerError::StartMismatch;return false;}
    std::vector<GridCell> replacement;if(!navigation.FindPath(start,route_.back(),replacement)||replacement.size()<2U||replacement.size()>kMaxRouteCells){lastError_=GridRouteFollowerError::ReplanFailed;return false;}
    route_=std::move(replacement);routeIndex_=1U;++routeRevision_;if(routeRevision_==0U)++routeRevision_;started_=false;lastError_=GridRouteFollowerError::None;return true;
}
} // namespace NeoEngine
