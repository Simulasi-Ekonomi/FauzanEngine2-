#include "Runtime/NeoRuntime.h"
#include "Runtime/RuntimeVerticalSliceGate.h"
#include <cassert>
int main(){
 NeoEngine::NeoRuntime runtime;
 NeoEngine::RuntimeConfig config{};
 config.farmNpcCount=1; config.renderWidth=64; config.renderHeight=48;
 assert(runtime.Initialize(config));
 assert(runtime.State() == NeoEngine::RuntimeState::Initialized);
 NeoEngine::VerticalSliceGateReceipt receipt{};
 assert(NeoEngine::RuntimeVerticalSliceGate::Validate(runtime,true,receipt));
 assert(receipt.initialized);
 assert(receipt.sceneValid);
 assert(receipt.ecsValid);
 assert(receipt.sceneECSConsistent && receipt.tickAccepted);
 assert(receipt.assetsValid);
 assert(receipt.resourcesValid);
 assert(receipt.replicationValid);
 assert(receipt.sceneECSRevisionValid);
 assert(receipt.sceneMeshRegistryValid);
 assert(receipt.sceneEntities == receipt.ecsEntities);
 assert(receipt.error == NeoEngine::VerticalSliceGateError::None);
 assert(runtime.Shutdown());
 assert(runtime.State() != NeoEngine::RuntimeState::Initialized);
 return 0;
}
