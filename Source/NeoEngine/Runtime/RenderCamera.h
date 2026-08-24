#pragma once

#include <cstdint>

namespace NeoEngine {

enum class RenderCameraMode : uint8_t { Orthographic, Perspective };
enum class RenderCameraError : uint8_t { None, InvalidConfiguration, BehindCamera, OutsideClip };
struct RenderPoint3 { float x = 0.0F, y = 0.0F, z = 0.0F; };
struct RenderCameraConfig { RenderCameraMode mode = RenderCameraMode::Orthographic; RenderPoint3 position{}; float orthographicHalfHeight = 5.0F; float verticalFovDegrees = 60.0F; float aspect = 1.0F; float nearPlane = 0.1F; float farPlane = 1000.0F; };

class RenderCamera {
public:
    bool Initialize(const RenderCameraConfig& config);
    bool Project(RenderPoint3 world, RenderPoint3& clip);
    [[nodiscard]] RenderCameraError LastError() const { return lastError_; }
    [[nodiscard]] const RenderCameraConfig& Config() const { return config_; }
private:
    RenderCameraConfig config_{}; RenderCameraError lastError_ = RenderCameraError::InvalidConfiguration; bool ready_ = false;
};

} // namespace NeoEngine
