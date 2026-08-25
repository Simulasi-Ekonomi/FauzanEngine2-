#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Runtime/GameplayPhysicsQuery.h"
#include "Threading/JobSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

int main() {
    using namespace NeoEngine;
    JobSystem::Get().Initialize(2); const auto fail=[](const char* stage) { std::fprintf(stderr,"GAMEPLAY_PHYSICS_QUERY_FAIL %s\n",stage); JobSystem::Get().Shutdown(); return 1; }; ArchetypeManager entities; XPBDPhysicsSystem physics; const uint32_t flags=COMP_POSITION|COMP_VELOCITY|COMP_COLLIDER;
    const EntityID staticEntity=entities.CreateEntity(flags); entities.SetPosX(staticEntity,2.0F); entities.SetRadius(staticEntity,0.5F); entities.SetInvMass(staticEntity,0.0F);
    physics.Step(entities,1.0F/60.0F); physics.SetEntityLayer(0,COLLISION_LAYER_STATIC);
    GameplayPhysicsQuery query; GameplayRayHit2 hit{}; const GameplayRay2 ray{0,0,1,0,10,COLLISION_LAYER_STATIC};
    if(!query.Raycast(physics,ray,hit)||query.LastError()!=GameplayPhysicsQueryError::None||hit.entity!=staticEntity||std::fabs(hit.distance-1.5F)>0.001F||std::fabs(hit.normalX+1.0F)>0.001F)return fail("static");
    const GameplayRayHit2 preserved=hit; if(query.Raycast(physics,{0,0,1,0,10,COLLISION_LAYER_DYNAMIC},hit)||query.LastError()!=GameplayPhysicsQueryError::NoHit||hit.entity!=preserved.entity)return fail("filtered");
    physics.SetEntityLayer(0,COLLISION_LAYER_DYNAMIC); if(!query.Raycast(physics,{0,0,1,0,10,COLLISION_LAYER_DYNAMIC},hit)||hit.entity!=staticEntity||std::fabs(hit.distance-1.5F)>0.001F)return fail("dynamic"); const GameplayRayHit2 dynamicPreserved=hit;
    if(query.Raycast(physics,{0,0,std::numeric_limits<float>::quiet_NaN(),0,10,COLLISION_LAYER_DYNAMIC},hit)||query.LastError()!=GameplayPhysicsQueryError::InvalidRay||hit.entity!=dynamicPreserved.entity)return fail("nan");
    if(query.Raycast(physics,{0,0,-1,0,10,COLLISION_LAYER_DYNAMIC},hit)||query.LastError()!=GameplayPhysicsQueryError::NoHit||hit.entity!=dynamicPreserved.entity)return fail("miss");
    if(query.Raycast(physics,{0,0,1,0,10,COLLISION_LAYER_NONE},hit)||query.LastError()!=GameplayPhysicsQueryError::InvalidMask||hit.entity!=dynamicPreserved.entity)return fail("mask");
    std::vector<EntityID> overlap; if(!query.OverlapCircle(physics,{2,0,0.25F,COLLISION_LAYER_DYNAMIC},overlap)||overlap.size()!=1U||overlap[0]!=staticEntity)return fail("overlap"); const std::vector<EntityID> overlapPreserved=overlap;
    if(!query.OverlapCircle(physics,{20,0,0.25F,COLLISION_LAYER_DYNAMIC},overlap)||!overlap.empty())return fail("empty"); overlap=overlapPreserved;
    if(query.OverlapCircle(physics,{0,0,-1,COLLISION_LAYER_DYNAMIC},overlap)||query.LastError()!=GameplayPhysicsQueryError::InvalidShape||overlap!=overlapPreserved)return fail("shape");
    if(query.OverlapCircle(physics,{0,0,1,COLLISION_LAYER_NONE},overlap)||query.LastError()!=GameplayPhysicsQueryError::InvalidMask||overlap!=overlapPreserved)return fail("overlapMask");
    const EntityID secondEntity=entities.CreateEntity(flags); entities.SetPosX(secondEntity,2.25F); entities.SetRadius(secondEntity,0.5F); entities.SetInvMass(secondEntity,0.0F); physics.Step(entities,1.0F/60.0F); physics.SetEntityLayer(0,COLLISION_LAYER_DYNAMIC); physics.SetEntityLayer(1,COLLISION_LAYER_DYNAMIC); std::vector<EntityID> sortedOverlap;
    if(!query.OverlapCircle(physics,{2,0,1.0F,COLLISION_LAYER_DYNAMIC},sortedOverlap)||sortedOverlap.size()!=2U||!std::is_sorted(sortedOverlap.begin(),sortedOverlap.end())||sortedOverlap[0]!=std::min(staticEntity,secondEntity)||sortedOverlap[1]!=std::max(staticEntity,secondEntity))return fail("sortedOverlap");
    JobSystem::Get().Shutdown(); std::printf("GAMEPLAY_PHYSICS_QUERY_SMOKE_OK static=1 dynamic=1 sorted=1 atomic=1 noStepWrite=1\n"); return 0;
}
