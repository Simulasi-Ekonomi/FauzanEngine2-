#pragma once

#include "SceneWorld.h"

#include <cstdint>

namespace NeoEngine {
enum class KinematicMotionError : uint8_t { None, NotInitialized, InvalidConfiguration, InvalidInput, MissingEntity, TransformFailed };
struct KinematicMotionConfig { float unitsPerSecond = 5.0F; float maxStepSeconds = 0.25F; bool alignYawToPlanarInput = false; };
struct KinematicPlanarInput { float x = 0.0F; float z = 0.0F; };
class KinematicMotionController {
public:
    bool Initialize(KinematicMotionConfig config = {});
    bool Step(SceneWorld& world,SceneEntity entity,KinematicPlanarInput input,float seconds);
    [[nodiscard]] KinematicMotionError LastError() const { return lastError_; }
    [[nodiscard]] bool IsReady() const { return initialized_; }
    [[nodiscard]] float UnitsPerSecond() const { return config_.unitsPerSecond; }
    [[nodiscard]] float MaxStepSeconds() const { return config_.maxStepSeconds; }
private:
    KinematicMotionConfig config_{};
    KinematicMotionError lastError_ = KinematicMotionError::NotInitialized;
    bool initialized_ = false;
};
} // namespace NeoEngine
