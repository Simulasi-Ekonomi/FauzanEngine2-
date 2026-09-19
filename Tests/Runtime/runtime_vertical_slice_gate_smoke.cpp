#include "Runtime/NeoRuntime.h"
#include "Runtime/RuntimeVerticalSliceGate.h"
#include <cassert>
int main(){
 NeoEngine::NeoRuntime runtime;
 NeoEngine::RuntimeConfig config{};
 config.farmNpcCount=1; config.renderWidth=64; config.renderHeight=48;
 assert(runtime.Initialize(config));
 NeoEngine::VerticalSliceGateReceipt receipt{};
 assert(NeoEngine::RuntimeVerticalSliceGate::Validate(runtime,true,receipt));
 assert(receipt.sceneECSConsistent && receipt.tickAccepted);
 assert(runtime.Shutdown());
 return 0;
}
