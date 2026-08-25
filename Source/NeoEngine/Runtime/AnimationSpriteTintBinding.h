#pragma once

#include <cstdint>

namespace NeoEngine {
enum class AnimationSpriteTintBindingError : uint8_t { None, NotInitialized, InvalidSample };
struct AnimationSpriteTintBindingConfig { uint32_t idleRgba = 0xFFFFFFFFU; uint32_t locomotionRgba = 0xFF80D0FFU; };

// Frame-local visual mapping only. It owns no SceneWorld, transform, route, or authority.
class AnimationSpriteTintBinding {
public:
    bool Initialize(AnimationSpriteTintBindingConfig config);
    bool Resolve(float normalizedSample, uint32_t& rgba) const;
    [[nodiscard]] bool IsReady() const { return initialized_; }
    [[nodiscard]] AnimationSpriteTintBindingError LastError() const { return lastError_; }
private:
    AnimationSpriteTintBindingConfig config_{};
    bool initialized_ = false;
    mutable AnimationSpriteTintBindingError lastError_ = AnimationSpriteTintBindingError::NotInitialized;
};
} // namespace NeoEngine
