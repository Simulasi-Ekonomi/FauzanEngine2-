#include "SceneECSBridge.h"
#include <limits>
namespace NeoEngine {
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
  ecs.SetTransform(id,t->x,t->y,t->z,t->rx,t->ry,t->rz,t->sx,t->sy,t->sz);
  if(old==map_.end()) { ecs.SetVelX(id,0.0F); ecs.SetVelZ(id,0.0F); }
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
 const auto entities=scene.AliveEntities();
 if(entities.size()!=map_.size()) return Rebuild(scene,ecs);
 for(const SceneEntity entity:entities){
  const uint32_t key=(static_cast<uint32_t>(entity.generation)<<16U)|entity.index;
  const auto it=map_.find(key);
  const Transform3* t=scene.GetTransform(entity);
  if(it==map_.end()||t==nullptr||!ecs.HasEntity(it->second)) return Rebuild(scene,ecs);
  const EntityID id=it->second;
  ecs.SetTransform(id,t->x,t->y,t->z,t->rx,t->ry,t->rz,t->sx,t->sy,t->sz);
 }
 receipt_.sceneCount=static_cast<uint32_t>(entities.size());
 receipt_.ecsCount=static_cast<uint32_t>(map_.size());
 receipt_.revision=ecs.GetPhysicsRevision();
 return true;
}
}
