#include "RenderCamera.h"

#include <cmath>

namespace NeoEngine {
bool RenderCamera::Initialize(const RenderCameraConfig& config) {
    const bool base = std::isfinite(config.position.x) && std::isfinite(config.position.y) && std::isfinite(config.position.z) && std::isfinite(config.aspect) && config.aspect > 0.0F && std::isfinite(config.nearPlane) && std::isfinite(config.farPlane) && config.nearPlane > 0.0F && config.farPlane > config.nearPlane;
    const bool orthographic = config.mode == RenderCameraMode::Orthographic && std::isfinite(config.orthographicHalfHeight) && config.orthographicHalfHeight > 0.0F;
    const bool perspective = config.mode == RenderCameraMode::Perspective && std::isfinite(config.verticalFovDegrees) && config.verticalFovDegrees > 1.0F && config.verticalFovDegrees < 179.0F;
    if (!base || (!orthographic && !perspective)) { ready_ = false; lastError_ = RenderCameraError::InvalidConfiguration; return false; }
    config_ = config; ready_ = true; lastError_ = RenderCameraError::None; return true;
}
bool RenderCamera::Project(RenderPoint3 world, RenderPoint3& clip) {
    if (!ready_) { lastError_ = RenderCameraError::InvalidConfiguration; return false; }
    const float x = world.x - config_.position.x, y = world.y - config_.position.y, z = world.z - config_.position.z;
    if (config_.mode == RenderCameraMode::Orthographic) { const float halfWidth = config_.orthographicHalfHeight * config_.aspect; clip = {x / halfWidth, y / config_.orthographicHalfHeight, (z - config_.nearPlane) / (config_.farPlane - config_.nearPlane)}; }
    else { if (z < config_.nearPlane) { lastError_ = RenderCameraError::BehindCamera; return false; } const float tanHalf = std::tan(config_.verticalFovDegrees * 0.00872664626F); clip = {x / (z * tanHalf * config_.aspect), y / (z * tanHalf), (z - config_.nearPlane) / (config_.farPlane - config_.nearPlane)}; }
    if (!std::isfinite(clip.x) || !std::isfinite(clip.y) || !std::isfinite(clip.z) || std::fabs(clip.x) > 1.0F || std::fabs(clip.y) > 1.0F || clip.z < 0.0F || clip.z > 1.0F) { lastError_ = RenderCameraError::OutsideClip; return false; }
    lastError_ = RenderCameraError::None; return true;
}
} // namespace NeoEngine
