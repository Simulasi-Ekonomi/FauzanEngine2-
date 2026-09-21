#include "SceneECSBridge.h"
#include <cmath>
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
  if(!t || !std::isfinite(t->x) || !std::isfinite(t->y) || !std::isfinite(t->z) || !std::isfinite(t->rx) || !std::isfinite(t->ry) || !std::isfinite(t->rz) || !std::isfinite(t->sx) || !std::isfinite(t->sy) || !std::isfinite(t->sz)) return false;
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
 if(entities.size()>std::numeric_limits<uint32_t>::max() || map_.size()>std::numeric_limits<uint32_t>::max()) return false;
 receipt_.sceneCount=static_cast<uint32_t>(entities.size());
 receipt_.ecsCount=static_cast<uint32_t>(map_.size());
 receipt_.revision=ecs.GetPhysicsRevision();
 return true;
}
}
