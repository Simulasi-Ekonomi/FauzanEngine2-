#include "TextureStreamer.h"
#include "GPUTextureCache.h"
namespace NeoEngine {
void TextureStreamer::StreamIn(const std::string& path){ static GPUTextureCache cache; if(!path.empty()) cache.LoadTexture(path); }
void TextureStreamer::StreamOut(const std::string& path){ static GPUTextureCache cache; if(!path.empty()) cache.Remove(path); }
}