#include "InputMotionBridge.h"

#include <array>

namespace NeoEngine {
bool InputMotionBridge::Initialize(InputMotionBindings bindings){
    const std::array<const std::string*,4> values{&bindings.forward,&bindings.backward,&bindings.left,&bindings.right};
    for(size_t index=0;index<values.size();++index){if(values[index]->empty()||values[index]->size()>64U){lastError_=InputMotionBridgeError::InvalidConfiguration;initialized_=false;return false;}for(size_t other=0;other<index;++other)if(*values[index]==*values[other]){lastError_=InputMotionBridgeError::InvalidConfiguration;initialized_=false;return false;}}
    bindings_=std::move(bindings);lastError_=InputMotionBridgeError::None;initialized_=true;return true;
}
bool InputMotionBridge::Step(const InputState& input,KinematicMotionController& controller,SceneWorld& world,SceneEntity entity,float seconds){
    if(!initialized_){lastError_=InputMotionBridgeError::NotInitialized;return false;}
    if(!input.HasAction(bindings_.forward)||!input.HasAction(bindings_.backward)||!input.HasAction(bindings_.left)||!input.HasAction(bindings_.right)){lastError_=InputMotionBridgeError::MissingAction;return false;}
    const float x=(input.Query(bindings_.right).pressed?1.0F:0.0F)-(input.Query(bindings_.left).pressed?1.0F:0.0F);
    const float z=(input.Query(bindings_.forward).pressed?1.0F:0.0F)-(input.Query(bindings_.backward).pressed?1.0F:0.0F);
    if(!controller.Step(world,entity,{x,z},seconds)){lastError_=InputMotionBridgeError::ControllerFailed;return false;}
    lastError_=InputMotionBridgeError::None;return true;
}
} // namespace NeoEngine
