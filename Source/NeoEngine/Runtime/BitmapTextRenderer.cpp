#include "BitmapTextRenderer.h"

#include "SoftwareRenderer.h"

#include <array>
#include <cmath>

namespace NeoEngine {
namespace {
using Glyph = std::array<uint8_t, 7>;
const Glyph* GlyphFor(char c) {
    static constexpr Glyph blank{0,0,0,0,0,0,0};
    static constexpr Glyph glyphs[] = {
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{31,4,4,4,4,4,31},{7,2,2,2,2,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14}
    };
    if (c == ' ') return &blank;
    if (c >= 'A' && c <= 'Z') return &glyphs[c - 'A'];
    if (c >= '0' && c <= '9') return &glyphs[26 + c - '0'];
    return nullptr;
}
bool DrawCell(SoftwareRenderer& renderer, uint16_t x, uint16_t y, uint8_t scale, uint32_t rgba, float clipDepth) {
    const float left=-1.0F+2.0F*static_cast<float>(x)/static_cast<float>(renderer.Width()-1U), right=-1.0F+2.0F*static_cast<float>(x+scale)/static_cast<float>(renderer.Width()-1U), top=1.0F-2.0F*static_cast<float>(y)/static_cast<float>(renderer.Height()-1U), bottom=1.0F-2.0F*static_cast<float>(y+scale)/static_cast<float>(renderer.Height()-1U);
    const RenderVertex a{left,bottom,clipDepth,rgba}, b{right,bottom,clipDepth,rgba}, c{right,top,clipDepth,rgba}, d{left,top,clipDepth,rgba};
    return renderer.DrawTriangle(a,b,c) && renderer.DrawTriangle(a,c,d);
}
} // namespace
bool BitmapTextRenderer::Draw(SoftwareRenderer& renderer, std::string_view text, uint16_t pixelX, uint16_t pixelY, uint8_t pixelScale, uint32_t rgba) { return DrawAtDepth(renderer, text, pixelX, pixelY, pixelScale, rgba, 0.0F); }
bool BitmapTextRenderer::DrawAtDepth(SoftwareRenderer& renderer, std::string_view text, uint16_t pixelX, uint16_t pixelY, uint8_t pixelScale, uint32_t rgba, float clipDepth) {
    if (text.empty()) { lastError_=BitmapTextError::EmptyString; return false; }
    if (text.size()>kMaxCharacters) { lastError_=BitmapTextError::StringTooLong; return false; }
    if (pixelScale==0 || pixelScale>16) { lastError_=BitmapTextError::InvalidScale; return false; }
    if (!std::isfinite(clipDepth) || clipDepth < 0.0F || clipDepth > 1.0F) { lastError_=BitmapTextError::InvalidDepth; return false; }
    const uint32_t width=static_cast<uint32_t>(text.size())*(kGlyphWidth+1U)*pixelScale-pixelScale, height=static_cast<uint32_t>(kGlyphHeight)*pixelScale;
    if(renderer.Width()==0 || renderer.Height()==0 || static_cast<uint32_t>(pixelX)+width>renderer.Width() || static_cast<uint32_t>(pixelY)+height>renderer.Height()) { lastError_=BitmapTextError::OutsideSurface; return false; }
    for(size_t index=0;index<text.size();++index) { const Glyph* glyph=GlyphFor(text[index]); if(!glyph){lastError_=BitmapTextError::UnsupportedCharacter;return false;} for(uint8_t row=0;row<kGlyphHeight;++row) for(uint8_t col=0;col<kGlyphWidth;++col) if(((*glyph)[row]&(1U<<(kGlyphWidth-1U-col)))!=0U&&!DrawCell(renderer,static_cast<uint16_t>(pixelX+index*(kGlyphWidth+1U)*pixelScale+col*pixelScale),static_cast<uint16_t>(pixelY+row*pixelScale),pixelScale,rgba,clipDepth)){lastError_=BitmapTextError::RasterFailed;return false;} }
    lastError_=BitmapTextError::None; return true;
}
} // namespace NeoEngine
