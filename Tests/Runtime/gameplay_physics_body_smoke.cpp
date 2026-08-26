#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Runtime/GameplayPhysicsBody.h"
#include "Runtime/GameplayPhysicsQuery.h"
#include "Threading/JobSystem.h"

#include <cmath>
#include <cstdio>

int main() {
    using namespace NeoEngine;
    JobSystem::Get().Initialize(2); const auto fail=[](const char* stage) { std::fprintf(stderr,"GAMEPLAY_PHYSICS_BODY_FAIL %s\n",stage); JobSystem::Get().Shutdown(); return 1; }; ArchetypeManager entities; GameplayPhysicsBodyBuilder builder; EntityID dynamic=999, statik=999;
    if(!builder.CreateCircleBody(entities,{GameplayPhysicsBodyType::Dynamic,2,0,3,0,0.5F,2},dynamic)||builder.LastError()!=GameplayPhysicsBodyError::None||dynamic==999||!builder.CreateCircleBody(entities,{GameplayPhysicsBodyType::Static,5,0,0,0,1,0},statik)||statik==999)return fail("create");
    GameplayCircleBodySnapshot snapshot{}; if(!builder.SnapshotCircleBody(entities,dynamic,snapshot)||builder.LastError()!=GameplayPhysicsBodyError::None||snapshot.type!=GameplayPhysicsBodyType::Dynamic||std::fabs(snapshot.positionX-2.0F)>0.001F||std::fabs(snapshot.velocityX-3.0F)>0.001F||std::fabs(snapshot.inverseMass-0.5F)>0.001F)return fail("snapshotDynamic"); if(!builder.SnapshotCircleBody(entities,statik,snapshot)||snapshot.type!=GameplayPhysicsBodyType::Static||snapshot.velocityX!=0.0F||snapshot.inverseMass!=0.0F)return fail("snapshotStatic");
    const uint64_t validRevision=entities.GetPhysicsRevision(); EntityID preserved=statik; GameplayCircleBodySnapshot preservedSnapshot{GameplayPhysicsBodyType::Dynamic,9,8,7,6,5,4}; if(builder.CreateCircleBody(entities,{GameplayPhysicsBodyType::Dynamic,0,0,0,0,-1,1},statik)||builder.LastError()!=GameplayPhysicsBodyError::InvalidConfiguration||statik!=preserved||entities.GetPhysicsRevision()!=validRevision||builder.SnapshotCircleBody(entities,99999U,preservedSnapshot)||builder.LastError()!=GameplayPhysicsBodyError::UnknownBody||preservedSnapshot.positionX!=9.0F||preservedSnapshot.inverseMass!=4.0F)return fail("invalid");
    bool dynamicOk=false, staticOk=false; for(ArchetypeChunk* chunk:entities.GetChunks<PositionComponent,VelocityComponent,ColliderComponent>()) for(size_t i=0;i<chunk->count;++i) { if(chunk->entities[i]==dynamic) dynamicOk=std::fabs(chunk->posX[i]-2.0F)<0.001F&&std::fabs(chunk->velX[i]-3.0F)<0.001F&&std::fabs(chunk->radius[i]-0.5F)<0.001F&&std::fabs(chunk->invMass[i]-0.5F)<0.001F; if(chunk->entities[i]==statik) staticOk=std::fabs(chunk->posX[i]-5.0F)<0.001F&&chunk->velX[i]==0.0F&&chunk->invMass[i]==0.0F; }
    XPBDPhysicsSystem physics; physics.Step(entities,1.0F/60.0F); GameplayPhysicsQuery query; GameplayRayHit2 hit{}; if(!dynamicOk||!staticOk||!query.Raycast(physics,{0,0,1,0,10,COLLISION_LAYER_DEFAULT},hit)||hit.entity!=dynamic)return fail("query");
    JobSystem::Get().Shutdown(); std::printf("GAMEPLAY_PHYSICS_BODY_SMOKE_OK dynamic=1 static=1 snapshot=1 ecs=1 query=1 invalid=1\n"); return 0;
}
