#include "RuntimeVerticalSliceGate.h"
#include <limits>
#include <cstdint>
namespace NeoEngine {
bool RuntimeVerticalSliceGate::Validate(NeoRuntime& runtime,bool executeTick,VerticalSliceGateReceipt& receipt){
 receipt={};

 if(runtime.State()!=RuntimeState::Initialized){receipt.error=VerticalSliceGateError::RuntimeNotInitialized;return false;}
 receipt.initialized=true;
 if(runtime.SceneECS().revision==0U || runtime.SceneECS().revision==std::numeric_limits<uint64_t>::max()){receipt.error=VerticalSliceGateError::SceneECSRevisionMismatch;return false;}
 receipt.sceneValid=runtime.Scene()!=nullptr;
 if(!receipt.sceneValid){receipt.error=VerticalSliceGateError::SceneMissing;return false;}
 receipt.ecsValid=runtime.ECS()!=nullptr;
 if(!receipt.ecsValid){receipt.error=VerticalSliceGateError::ECSMissing;return false;}
 receipt.sceneEntities=runtime.Scene()->AliveCount();
 if(receipt.sceneEntities>std::numeric_limits<uint32_t>::max()){receipt.error=VerticalSliceGateError::SceneECSMismatch;return false;}
 if(runtime.SceneECS().sceneCount > std::numeric_limits<uint32_t>::max() || runtime.SceneECS().ecsCount > std::numeric_limits<uint32_t>::max()){receipt.error=VerticalSliceGateError::SceneECSMismatch;return false;}
 receipt.ecsEntities=runtime.SceneECS().ecsCount;
 receipt.sceneECSConsistent=runtime.SceneECS().sceneCount==receipt.sceneEntities &&
                              runtime.SceneECS().ecsCount==receipt.sceneEntities;
 receipt.sceneECSRevisionValid=runtime.SceneECS().revision==runtime.ECS()->GetPhysicsRevision();
 if(!receipt.sceneECSConsistent){receipt.error=VerticalSliceGateError::SceneECSMismatch;return false;}
 if(!receipt.sceneECSRevisionValid){receipt.error=VerticalSliceGateError::SceneECSRevisionMismatch;return false;}
 receipt.sceneMeshRegistryValid=runtime.SceneMeshes()!=nullptr;
 if(!receipt.sceneMeshRegistryValid){receipt.error=VerticalSliceGateError::SceneMeshMissing;return false;}
 receipt.assetsValid=runtime.Assets()!=nullptr;
 receipt.resourcesValid=runtime.Resources()!=nullptr;
 if(!receipt.assetsValid || !receipt.resourcesValid){receipt.error=VerticalSliceGateError::AssetsMissing;return false;}
 receipt.replicationValid=runtime.Replication()!=nullptr;
 if(!receipt.replicationValid){ receipt.error=VerticalSliceGateError::ReplicationMissing; return false; }
 if(!receipt.assetsValid){receipt.error=VerticalSliceGateError::AssetsMissing;return false;}
 if(!receipt.resourcesValid){receipt.error=VerticalSliceGateError::ResourcesMissing;return false;}
 if(!receipt.replicationValid){receipt.error=VerticalSliceGateError::ReplicationMissing;return false;}
 if(executeTick && runtime.SceneECS().revision==0U){receipt.error=VerticalSliceGateError::SceneECSRevisionMismatch;return false;}
 if(executeTick && runtime.State()!=RuntimeState::Initialized){receipt.error=VerticalSliceGateError::RuntimeNotInitialized;return false;}
 if(executeTick){
  receipt.tickAccepted=runtime.Tick();
  if(!receipt.tickAccepted){receipt.error=VerticalSliceGateError::TickRejected;return false;}
  if(runtime.State()!=RuntimeState::Initialized){receipt.error=VerticalSliceGateError::RuntimeNotInitialized;return false;}
  if(runtime.Scene()==nullptr || runtime.ECS()==nullptr){receipt.error=VerticalSliceGateError::SceneECSMismatch;return false;}
  const auto postTickSceneCount=runtime.Scene()->AliveCount();
  if(postTickSceneCount>std::numeric_limits<uint32_t>::max() || runtime.SceneECS().sceneCount!=postTickSceneCount || runtime.SceneECS().ecsCount!=postTickSceneCount){receipt.error=VerticalSliceGateError::SceneECSMismatch;return false;}
  if(runtime.SceneECS().revision!=runtime.ECS()->GetPhysicsRevision()){receipt.error=VerticalSliceGateError::SceneECSRevisionMismatch;return false;}
  if(runtime.SceneMeshes()==nullptr || runtime.Assets()==nullptr || runtime.Resources()==nullptr || runtime.Replication()==nullptr){receipt.error=VerticalSliceGateError::AssetsMissing;return false;}
 }
 receipt.error=VerticalSliceGateError::None;
 return true;
}
}
