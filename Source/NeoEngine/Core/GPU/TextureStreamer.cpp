#include <cassert>
#include "TextureStreamer.h"
#include "GPUTextureCache.h"

namespace NeoEngine
{

void TextureStreamer::StreamIn(const std::string& path)
{
    GPUTextureCache cache;
    cache.LoadTexture(path);
}

void TextureStreamer::StreamOut(const std::string& path)
{
    GPUTextureCache cache;
    cache.Remove(path);
}

}
