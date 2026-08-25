#pragma once

#include <cstdint>

namespace NeoEngine {
enum class FlipbookFrameSelectorError : uint8_t { None, NotInitialized, InvalidConfiguration, InvalidSample };
struct SpriteSourceRect { uint16_t x = 0U, y = 0U, width = 0U, height = 0U; };
struct FlipbookFrameSelectorConfig { uint16_t textureWidth = 0U, textureHeight = 0U, frameWidth = 0U, frameHeight = 0U, frameCount = 0U; };
class FlipbookFrameSelector { public: bool Initialize(FlipbookFrameSelectorConfig config); bool Select(float normalizedSample, SpriteSourceRect& output) const; [[nodiscard]] FlipbookFrameSelectorError LastError() const{return lastError_;} private: FlipbookFrameSelectorConfig config_{}; uint16_t columns_ = 0U; bool initialized_ = false; mutable FlipbookFrameSelectorError lastError_ = FlipbookFrameSelectorError::NotInitialized; };
} // namespace NeoEngine
