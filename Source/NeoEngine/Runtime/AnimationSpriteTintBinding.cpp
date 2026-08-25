#include "Runtime/AnimationSpriteTintBinding.h"

#include <cmath>

namespace NeoEngine {
namespace {
uint8_t Channel(uint32_t rgba, uint32_t shift) { return static_cast<uint8_t>((rgba >> shift) & 0xFFU); }
uint8_t Blend(uint8_t from, uint8_t to, float weight) { return static_cast<uint8_t>(std::lround(static_cast<float>(from) + (static_cast<float>(to) - static_cast<float>(from)) * weight)); }
uint32_t BlendRgba(uint32_t from, uint32_t to, float weight) { return (static_cast<uint32_t>(Blend(Channel(from,24U),Channel(to,24U),weight)) << 24U) | (static_cast<uint32_t>(Blend(Channel(from,16U),Channel(to,16U),weight)) << 16U) | (static_cast<uint32_t>(Blend(Channel(from,8U),Channel(to,8U),weight)) << 8U) | static_cast<uint32_t>(Blend(Channel(from,0U),Channel(to,0U),weight)); }
}
bool AnimationSpriteTintBinding::Initialize(AnimationSpriteTintBindingConfig config) { config_ = config; initialized_ = true; lastError_ = AnimationSpriteTintBindingError::None; return true; }
bool AnimationSpriteTintBinding::Resolve(float normalizedSample, uint32_t& rgba) const { if (!initialized_) { lastError_ = AnimationSpriteTintBindingError::NotInitialized; return false; } if (!std::isfinite(normalizedSample) || normalizedSample < 0.0F || normalizedSample > 1.0F) { lastError_ = AnimationSpriteTintBindingError::InvalidSample; return false; } rgba = BlendRgba(config_.idleRgba, config_.locomotionRgba, normalizedSample); lastError_ = AnimationSpriteTintBindingError::None; return true; }
} // namespace NeoEngine
