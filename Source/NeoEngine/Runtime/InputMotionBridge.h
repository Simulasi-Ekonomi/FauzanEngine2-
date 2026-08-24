#pragma once

#include "InputState.h"
#include "KinematicMotionController.h"

#include <cstdint>
#include <string>

namespace NeoEngine {
enum class InputMotionBridgeError : uint8_t { None, InvalidConfiguration, NotInitialized, MissingAction, ControllerFailed };
struct InputMotionBindings { std::string forward = "move_forward"; std::string backward = "move_backward"; std::string left = "move_left"; std::string right = "move_right"; };
class InputMotionBridge {
public:
    bool Initialize(InputMotionBindings bindings = {});
    bool Step(const InputState& input,KinematicMotionController& controller,SceneWorld& world,SceneEntity entity,float seconds);
    [[nodiscard]] InputMotionBridgeError LastError() const { return lastError_; }
    [[nodiscard]] bool IsReady() const { return initialized_; }
private:
    InputMotionBindings bindings_{};
    InputMotionBridgeError lastError_ = InputMotionBridgeError::NotInitialized;
    bool initialized_ = false;
};
} // namespace NeoEngine
