#include "Runtime/BitmapTextRenderer.h"
#include "Runtime/SoftwareRenderer.h"

#include <cstdio>
#include <string>

int main() {
    using namespace NeoEngine; SoftwareRenderer renderer; BitmapTextRenderer text;
    if(!renderer.Initialize(192,48)||!renderer.Clear(0xFF000000)||!text.Draw(renderer,"FARM 2026",4,4,3,0xFFFFFFFF)||renderer.FrameHash()==0||renderer.PixelAt(5,5)==0xFF000000) return 1;
    const uint64_t hash=renderer.FrameHash(); const std::string overflow(129,'A');
    if(text.Draw(renderer,"F@RM",4,4,2,0xFFFFFFFF)||text.LastError()!=BitmapTextError::UnsupportedCharacter||text.Draw(renderer,overflow,0,0,1,0xFFFFFFFF)||text.LastError()!=BitmapTextError::StringTooLong||text.DrawAtDepth(renderer,"A",4,4,1,0xFFFFFFFF,-0.01F)||text.LastError()!=BitmapTextError::InvalidDepth||text.Draw(renderer,"A",191,47,2,0xFFFFFFFF)||text.LastError()!=BitmapTextError::OutsideSurface) return 1;
    std::printf("BITMAP_TEXT_RENDERER_SMOKE_OK glyphs=9 bounds=1 depth=1 unsupported=1 overflow=1 hash=%llu\n",static_cast<unsigned long long>(hash)); return 0;
}
