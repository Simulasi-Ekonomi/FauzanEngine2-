#include "Runtime/SceneWorld.h"

#include <cmath>
#include <cstring>
#include <cstdio>
#include <limits>

int main(){
    using namespace NeoEngine;
    SceneWorld world;
    SceneEntity parent{},child{},grandchild{};
    if(!world.Create(parent)||!world.Create(child)||!world.Create(grandchild)||!world.SetTransform(parent,{10,1,0,0,0,0,2,2,2})||!world.SetParent(child,parent)||!world.SetTransform(child,{2,3,4})||!world.SetParent(grandchild,child)||!world.SetTransform(grandchild,{1,0,0})||world.SetParent(parent,grandchild)||world.AliveCount()!=3)return 1;
    const Transform3* composed=world.GetTransform(grandchild);const Transform3* local=world.GetLocalTransform(grandchild);
    if(!composed||!local||composed->x!=16.0F||composed->y!=7.0F||local->x!=1.0F)return 1;
    if(!world.SetTransform(parent,{20,0,0,0,0,0,1,1,1})||!world.GetTransform(grandchild)||world.GetTransform(grandchild)->x!=23.0F||world.GetTransform(grandchild)->y!=3.0F)return 1;
    SceneWorld rotated;
    SceneEntity rotatedParent{},rotatedChild{},rotatedGrandchild{};
    if(!rotated.Create(rotatedParent)||!rotated.Create(rotatedChild)||!rotated.Create(rotatedGrandchild)||!rotated.SetTransform(rotatedParent,{0,0,0,0,0,1.57079632679F,2,2,2})||!rotated.SetParent(rotatedChild,rotatedParent)||!rotated.SetTransform(rotatedChild,{1,0,0})||!rotated.SetParent(rotatedGrandchild,rotatedChild)||!rotated.SetTransform(rotatedGrandchild,{1,0,0}))return 1;
    const Transform3* rotatedChildWorld=rotated.GetTransform(rotatedChild);const Transform3* rotatedGrandchildWorld=rotated.GetTransform(rotatedGrandchild);
    if(!rotatedChildWorld||!rotatedGrandchildWorld||std::fabs(rotatedChildWorld->x)>0.0001F||std::fabs(rotatedChildWorld->y-2.0F)>0.0001F||std::fabs(rotatedGrandchildWorld->x)>0.0001F||std::fabs(rotatedGrandchildWorld->y-4.0F)>0.0001F)return 1;
    SceneWorld rotatedRestored;const auto rotatedBytes=rotated.Serialize();
    if(!rotatedRestored.Deserialize(rotatedBytes)||!rotatedRestored.GetTransform(rotatedGrandchild)||std::fabs(rotatedRestored.GetTransform(rotatedGrandchild)->y-4.0F)>0.0001F)return 1;
    auto bytes=world.Serialize();SceneWorld restored;
    if(!restored.Deserialize(bytes)||restored.AliveCount()!=3||!restored.GetTransform(grandchild)||restored.GetTransform(grandchild)->x!=23.0F||!restored.Destroy(parent)||restored.GetTransform(parent)||!restored.GetTransform(child)||restored.GetTransform(child)->x!=2.0F)return 1;
    const float childX=restored.GetTransform(child)->x;
    if(restored.SetTransform(child,{std::numeric_limits<float>::infinity(),0,0})||restored.LastError()!=SceneWorldError::InvalidTransform||restored.GetTransform(child)->x!=childX)return 1;
    std::vector<uint8_t> corrupted=bytes;const float infinity=std::numeric_limits<float>::infinity();std::memcpy(corrupted.data()+14U,&infinity,sizeof(infinity));
    if(restored.Deserialize(corrupted)||restored.LastError()!=SceneWorldError::InvalidTransform||restored.GetTransform(child)->x!=childX)return 1;
    std::printf("SCENE_WORLD_SMOKE_OK entities=%u hierarchy=1 rotation=1 validation=1 dirty=1 localWorld=1 bytes=%zu\n",restored.AliveCount(),bytes.size());
}
