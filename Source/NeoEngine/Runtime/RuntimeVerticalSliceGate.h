#pragma once
#include "NeoRuntime.h"
#include <cstdint>
namespace NeoEngine {
enum class VerticalSliceGateError : uint8_t {
 None, RuntimeNotInitialized, SceneMissing, ECSMissing, SceneECSMismatch,
 AssetsMissing, ResourcesMissing, ReplicationMissing, TickRejected
};
struct VerticalSliceGateReceipt {
 bool initialized=false;
 bool sceneValid=false;
 bool ecsValid=false;
 bool sceneECSConsistent=false;
 bool assetsValid=false;
 bool resourcesValid=false;
 bool replicationValid=false;
 bool tickAccepted=false;
 uint32_t sceneEntities=0;
 uint32_t ecsEntities=0;
 VerticalSliceGateError error=VerticalSliceGateError::None;
};
class RuntimeVerticalSliceGate {
public:
 static bool Validate(NeoRuntime& runtime, bool executeTick, VerticalSliceGateReceipt& receipt);
};
}
