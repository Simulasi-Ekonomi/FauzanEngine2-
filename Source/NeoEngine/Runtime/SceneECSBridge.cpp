#include "SceneECSBridge.h"
#include <limits>
#include <unordered_map>

namespace NeoEngine {
namespace {
bool HasMeshForEntity(const SceneMeshAdapter& meshes, SceneEntity entity, const SceneMeshInstance** outInstance) {
    for (const SceneMeshInstance& instance : meshes.Instances()) {
        if (instance.entity == entity) {
            if (outInstance != nullptr) *outInstance = &instance;
            return true;
        }
    }
    if (outInstance != nullptr) *outInstance = nullptr;
    return false;
}
}

EntityID SceneECSBridge::ECSId(SceneEntity entity) const {
 const uint32_t key=(static_cast<uint32_t>(entity.generation)<<16U)|entity.index;
 const auto it=map_.find(key);
 return it==map_.end()?std::numeric_limits<EntityID>::max():it->second;
}

bool SceneECSBridge::Rebuild(SceneWorld& scene, ArchetypeManager& ecs) {
 const auto entities=scene.AliveEntities();
 std::unordered_map<uint32_t,EntityID> next;
 next.reserve(entities.size());
 for(const SceneEntity entity:entities){
  const Transform3* t=scene.GetTransform(entity);
  if(!t) return false;
  const uint32_t key=(static_cast<uint32_t>(entity.generation)<<16U)|entity.index;
  const auto old=map_.find(key);
  const EntityID id=old==map_.end()?ecs.CreateEntity(COMP_POSITION|COMP_ROTATION):old->second;
  if(id==std::numeric_limits<EntityID>::max()||!ecs.HasEntity(id)) return false;
  ecs.SetPosX(id,t->x); ecs.SetPosZ(id,t->z);
  if(old==map_.end()) ecs.SetVelX(id,0.0F);
  next.emplace(key,id);
 }
 for(const auto& [key,id]:map_) if(!next.contains(key)&&ecs.HasEntity(id)) ecs.DestroyEntity(id);
 map_.swap(next);
 receipt_.sceneCount=static_cast<uint32_t>(entities.size());
 receipt_.ecsCount=static_cast<uint32_t>(map_.size());
 receipt_.revision=ecs.GetPhysicsRevision();
 return true;
}

bool SceneECSBridge::Sync(SceneWorld& scene, ArchetypeManager& ecs, const SceneMeshAdapter& meshes) {
 const auto entities=scene.AliveEntities();
 std::unordered_map<uint32_t,EntityID> next;
 next.reserve(entities.size());

 for (const SceneEntity entity : entities) {
  const Transform3* transform=scene.GetTransform(entity);
  if (transform==nullptr) return false;
  const uint32_t key=(static_cast<uint32_t>(entity.generation)<<16U)|entity.index;
  const auto old=map_.find(key);
  EntityID id=old==map_.end()?ecs.CreateEntity(COMP_POSITION|COMP_ROTATION):old->second;
  if (id==std::numeric_limits<EntityID>::max() || !ecs.HasEntity(id)) return false;

  uint32_t mask=ecs.GetComponentMask(id);
  const SceneMeshInstance* mesh=nullptr;
  const bool hasMesh=HasMeshForEntity(meshes,entity,&mesh);
  if (hasMesh) mask|=COMP_MESH;
  else mask&=~COMP_MESH;
  if (mask==0U) mask=COMP_POSITION|COMP_ROTATION;
  ecs.SetComponentMask(id,mask);
  ecs.SetPosX(id,transform->x);
  ecs.SetPosY(id,transform->y);
  ecs.SetPosZ(id,transform->z);
  ecs.SetTransform(id,transform->x,transform->y,transform->z,transform->rx,transform->ry,transform->rz,transform->sx,transform->sy,transform->sz);
  if (old==map_.end()) ecs.SetVelX(id,0.0F);
  if (hasMesh && mesh!=nullptr) {
      if (mesh->sourceHash==0U || mesh->sourceMaterialHash==0U) return false;
      ecs.SetMeshAssetIdentity(id,mesh->sourceHash,mesh->sourceMaterialHash);
  }
  next.emplace(key,id);
 }

 for(const auto& [key,id]:map_) if(!next.contains(key)&&ecs.HasEntity(id)) ecs.DestroyEntity(id);
 map_.swap(next);
 receipt_.sceneCount=static_cast<uint32_t>(entities.size());
 receipt_.ecsCount=static_cast<uint32_t>(map_.size());
 receipt_.revision=ecs.GetPhysicsRevision();
 return true;
}

bool SceneECSBridge::Sync(SceneWorld& scene, ArchetypeManager& ecs) {
 return Rebuild(scene,ecs);
}
} // namespace NeoEngine
