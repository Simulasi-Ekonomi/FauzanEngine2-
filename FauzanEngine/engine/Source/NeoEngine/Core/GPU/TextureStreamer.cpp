#include "TextureStreamer.h"
#include "GPUTextureCache.h"
namespace NeoEngine { namespace { GPUTextureCache& Cache(){static GPUTextureCache c;return c;} } void TextureStreamer::StreamIn(const std::string&p){if(!p.empty())Cache().LoadTexture(p);} void TextureStreamer::StreamOut(const std::string&p){if(!p.empty())Cache().Remove(p);} }