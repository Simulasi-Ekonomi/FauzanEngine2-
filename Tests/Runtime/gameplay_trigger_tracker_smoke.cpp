#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Runtime/GameplayTriggerTracker.h"
#include "Threading/JobSystem.h"

#include <cstdio>

int main() {
    using namespace NeoEngine; JobSystem::Get().Initialize(2); const auto fail=[](const char* stage){std::fprintf(stderr,"GAMEPLAY_TRIGGER_TRACKER_FAIL %s\n",stage);JobSystem::Get().Shutdown();return 1;}; XPBDPhysicsSystem emptyPhysics;GameplayTriggerTracker uninitialized;if(uninitialized.Update(emptyPhysics)||uninitialized.LastError()!=GameplayTriggerTrackerError::NotInitialized)return fail("uninitialized");
    ArchetypeManager entities;const EntityID nearEntity=entities.CreateEntity(COMP_POSITION|COMP_VELOCITY|COMP_COLLIDER);entities.SetPosX(nearEntity,0.0F);entities.SetRadius(nearEntity,0.25F);entities.SetInvMass(nearEntity,0.0F);const EntityID farEntity=entities.CreateEntity(COMP_POSITION|COMP_VELOCITY|COMP_COLLIDER);entities.SetPosX(farEntity,5.0F);entities.SetRadius(farEntity,0.25F);entities.SetInvMass(farEntity,0.0F);XPBDPhysicsSystem physics;physics.Step(entities,1.0F/60.0F);physics.SetEntityLayer(0,COLLISION_LAYER_STATIC);physics.SetEntityLayer(1,COLLISION_LAYER_STATIC);
    GameplayTriggerTracker tracker;if(!tracker.Initialize({0,0,0.5F,COLLISION_LAYER_STATIC})||!tracker.Update(physics)||tracker.ActiveEntities().size()!=1U||tracker.ActiveEntities()[0]!=nearEntity||tracker.LastDelta().entered.size()!=1U||tracker.LastDelta().entered[0]!=nearEntity||!tracker.LastDelta().exited.empty())return fail("enter");
    if(!tracker.Update(physics)||tracker.LastDelta().entered.size()!=0U||tracker.LastDelta().exited.size()!=0U)return fail("steady");entities.SetPosX(nearEntity,2.0F);physics.Step(entities,1.0F/60.0F);if(!tracker.Update(physics)||!tracker.ActiveEntities().empty()||tracker.LastDelta().exited.size()!=1U||tracker.LastDelta().exited[0]!=nearEntity)return fail("exit");
    ArchetypeManager crowdedEntities;for(uint32_t i=0;i<GameplayPhysicsQuery::kMaxOverlapHits+1U;++i){const EntityID id=crowdedEntities.CreateEntity(COMP_POSITION|COMP_VELOCITY|COMP_COLLIDER);crowdedEntities.SetRadius(id,0.1F);crowdedEntities.SetInvMass(id,0.0F);}XPBDPhysicsSystem crowdedPhysics;crowdedPhysics.Step(crowdedEntities,1.0F/60.0F);for(uint32_t i=0;i<GameplayPhysicsQuery::kMaxOverlapHits+1U;++i)crowdedPhysics.SetEntityLayer(i,COLLISION_LAYER_STATIC);if(tracker.Update(crowdedPhysics)||tracker.LastError()!=GameplayTriggerTrackerError::QueryFailed||!tracker.ActiveEntities().empty()||!tracker.LastDelta().entered.empty()||tracker.LastDelta().exited.size()!=1U)return fail("atomic");
    JobSystem::Get().Shutdown();std::printf("GAMEPLAY_TRIGGER_TRACKER_SMOKE_OK enter=1 exit=1 sorted=1 atomic=1\n");return 0;
}
