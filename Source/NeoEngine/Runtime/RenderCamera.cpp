#include "RenderCamera.h"

#include <cmath>

namespace NeoEngine {
namespace {
bool Finite(RenderPoint3 value) { return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z); }
float Dot(RenderPoint3 first, RenderPoint3 second) { return first.x * second.x + first.y * second.y + first.z * second.z; }
RenderPoint3 Cross(RenderPoint3 first, RenderPoint3 second) { return {first.y * second.z - first.z * second.y, first.z * second.x - first.x * second.z, first.x * second.y - first.y * second.x}; }
bool Normalize(RenderPoint3 value, RenderPoint3& normalized) { const float lengthSquared = Dot(value, value); if (!std::isfinite(lengthSquared) || lengthSquared <= 1e-12F) return false; const float inverseLength = 1.0F / std::sqrt(lengthSquared); normalized = {value.x * inverseLength, value.y * inverseLength, value.z * inverseLength}; return Finite(normalized); }
}
bool RenderCamera::Initialize(const RenderCameraConfig& config) {
    const bool base = std::isfinite(config.position.x) && std::isfinite(config.position.y) && std::isfinite(config.position.z) && std::isfinite(config.aspect) && config.aspect > 0.0F && std::isfinite(config.nearPlane) && std::isfinite(config.farPlane) && config.nearPlane > 0.0F && config.farPlane > config.nearPlane;
    const bool orthographic = config.mode == RenderCameraMode::Orthographic && std::isfinite(config.orthographicHalfHeight) && config.orthographicHalfHeight > 0.0F;
    const bool perspective = config.mode == RenderCameraMode::Perspective && std::isfinite(config.verticalFovDegrees) && config.verticalFovDegrees > 1.0F && config.verticalFovDegrees < 179.0F;
    if (!base || (!orthographic && !perspective)) { ready_ = false; lastError_ = RenderCameraError::InvalidConfiguration; return false; }
    RenderPoint3 forward{};
    RenderPoint3 right{};
    RenderPoint3 correctedUp{};
    if (!Finite(config.forward) || !Finite(config.up) || !Normalize(config.forward, forward) || !Normalize(Cross(config.up, forward), right) || !Normalize(Cross(forward, right), correctedUp)) { ready_ = false; lastError_ = RenderCameraError::InvalidOrientation; return false; }
    config_ = config;
    config_.forward = forward;
    right_ = right;
    correctedUp_ = correctedUp;
    ready_ = true; lastError_ = RenderCameraError::None; return true;
}
bool RenderCamera::WorldToCamera(RenderPoint3 world, RenderPoint3& cameraSpace) {
    if (!ready_ || !Finite(world)) { lastError_ = RenderCameraError::InvalidConfiguration; return false; }
    const RenderPoint3 relative{world.x - config_.position.x, world.y - config_.position.y, world.z - config_.position.z};
    cameraSpace = {Dot(relative, right_), Dot(relative, correctedUp_), Dot(relative, config_.forward)};
    if (!Finite(cameraSpace)) { lastError_ = RenderCameraError::InvalidConfiguration; return false; }
    return true;
}
bool RenderCamera::Project(RenderPoint3 world, RenderPoint3& clip) {
    RenderPoint3 cameraSpace{};
    if (!WorldToCamera(world, cameraSpace)) return false;
    const float x = cameraSpace.x, y = cameraSpace.y, z = cameraSpace.z;
    if (config_.mode == RenderCameraMode::Orthographic) { const float halfWidth = config_.orthographicHalfHeight * config_.aspect; clip = {x / halfWidth, y / config_.orthographicHalfHeight, (z - config_.nearPlane) / (config_.farPlane - config_.nearPlane)}; }
    else { if (z < config_.nearPlane) { lastError_ = RenderCameraError::BehindCamera; return false; } const float tanHalf = std::tan(config_.verticalFovDegrees * 0.00872664626F); clip = {x / (z * tanHalf * config_.aspect), y / (z * tanHalf), (z - config_.nearPlane) / (config_.farPlane - config_.nearPlane)}; }
    if (!std::isfinite(clip.x) || !std::isfinite(clip.y) || !std::isfinite(clip.z) || std::fabs(clip.x) > 1.0F || std::fabs(clip.y) > 1.0F || clip.z < 0.0F || clip.z > 1.0F) { lastError_ = RenderCameraError::OutsideClip; return false; }
    lastError_ = RenderCameraError::None; return true;
}
bool RenderCamera::SphereIntersectsFrustum(RenderPoint3 center, float radius) {
    RenderPoint3 view{};
    if (!WorldToCamera(center, view) || !std::isfinite(radius) || radius < 0.0F) { lastError_ = RenderCameraError::InvalidConfiguration; return false; }
    if (view.z + radius < config_.nearPlane || view.z - radius > config_.farPlane) { lastError_ = RenderCameraError::None; return false; }
    if (config_.mode == RenderCameraMode::Orthographic) {
        const float halfHeight = config_.orthographicHalfHeight, halfWidth = halfHeight * config_.aspect;
        lastError_ = RenderCameraError::None;
        return !(view.x - radius > halfWidth || view.x + radius < -halfWidth || view.y - radius > halfHeight || view.y + radius < -halfHeight);
    }
    if (view.z - radius <= config_.nearPlane) { lastError_ = RenderCameraError::None; return true; }
    const float halfHeight = (view.z + radius) * std::tan(config_.verticalFovDegrees * 0.00872664626F), halfWidth = halfHeight * config_.aspect;
    lastError_ = RenderCameraError::None;
    return !(view.x - radius > halfWidth || view.x + radius < -halfWidth || view.y - radius > halfHeight || view.y + radius < -halfHeight);
}
} // namespace NeoEngine
