#include "Runtime/GameplayPhysicsBody.h"
#include "Runtime/ScenePhysicsPoseSync.h"

#include <cmath>
#include <cstdio>

int main() {
    using namespace NeoEngine; ArchetypeManager entities; GameplayPhysicsBodyBuilder bodies; EntityID physicsA=0,physicsB=0;if(!bodies.CreateCircleBody(entities,{GameplayPhysicsBodyType::Static,0,0,0,0,0.5F,0},physicsA)||!bodies.CreateCircleBody(entities,{GameplayPhysicsBodyType::Static,0,0,0,0,0.5F,0},physicsB))return 1;
    SceneWorld world;SceneEntity sceneA{},sceneB{};if(!world.Create(sceneA)||!world.Create(sceneB)||!world.SetTransform(sceneA,{2,0,3,0,0,0,1,1,1})||!world.SetTransform(sceneB,{4,0,5,0,0,0,1,1,1})||!world.UpdateTransforms())return 1;ScenePhysicsPoseSync sync;if(!sync.Bind(sceneA,physicsA)||!sync.Bind(sceneB,physicsB)||sync.Bind(sceneA,physicsA)||sync.LastError()!=ScenePhysicsPoseSyncError::DuplicateSceneEntity||!sync.Sync(world,entities))return 1;
    float ax=0,az=0,bx=0,bz=0;for(ArchetypeChunk* chunk:entities.GetChunks<PositionComponent,VelocityComponent,ColliderComponent>())for(size_t i=0;i<chunk->count;++i){if(chunk->entities[i]==physicsA){ax=chunk->posX[i];az=chunk->posZ[i];}if(chunk->entities[i]==physicsB){bx=chunk->posX[i];bz=chunk->posZ[i];}}if(std::fabs(ax-2.0F)>0.001F||std::fabs(az-3.0F)>0.001F||std::fabs(bx-4.0F)>0.001F||std::fabs(bz-5.0F)>0.001F)return 1;
    if(!world.SetTransform(sceneA,{9,0,9,0,0,0,1,1,1})||!world.UpdateTransforms()||!world.Destroy(sceneB)||sync.Sync(world,entities)||sync.LastError()!=ScenePhysicsPoseSyncError::MissingWorldTransform)return 1;for(ArchetypeChunk* chunk:entities.GetChunks<PositionComponent,VelocityComponent,ColliderComponent>())for(size_t i=0;i<chunk->count;++i)if(chunk->entities[i]==physicsA&&(std::fabs(chunk->posX[i]-2.0F)>0.001F||std::fabs(chunk->posZ[i]-3.0F)>0.001F))return 1;
    std::printf("SCENE_PHYSICS_POSE_SYNC_SMOKE_OK bindings=2 sync=1 atomic=1 oneWay=1\n");return 0;
}
