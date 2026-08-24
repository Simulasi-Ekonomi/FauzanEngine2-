#pragma once

#include <cstdint>
#include <string_view>

namespace NeoEngine {
class SoftwareRenderer;
enum class BitmapTextError : uint8_t { None, EmptyString, StringTooLong, InvalidScale, InvalidDepth, UnsupportedCharacter, OutsideSurface, RasterFailed };
class BitmapTextRenderer {
public:
    static constexpr uint16_t kMaxCharacters = 128;
    static constexpr uint8_t kGlyphWidth = 5;
    static constexpr uint8_t kGlyphHeight = 7;
    bool Draw(SoftwareRenderer& renderer, std::string_view text, uint16_t pixelX, uint16_t pixelY, uint8_t pixelScale, uint32_t rgba);
    bool DrawAtDepth(SoftwareRenderer& renderer, std::string_view text, uint16_t pixelX, uint16_t pixelY, uint8_t pixelScale, uint32_t rgba, float clipDepth);
    [[nodiscard]] BitmapTextError LastError() const { return lastError_; }
private:
    BitmapTextError lastError_ = BitmapTextError::None;
};
} // namespace NeoEngine
