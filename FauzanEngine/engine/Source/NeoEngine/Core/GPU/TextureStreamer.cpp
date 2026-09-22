#include "TextureStreamer.h"
#include "GPUTextureCache.h"
namespace NeoEngine {
void TextureStreamer::StreamIn(const std::string& path){
    if(path.empty()) return;
    GPUTextureCache cache; cache.LoadTexture(path);
}
void TextureStreamer::StreamOut(const std::string& path){
    if(path.empty()) return;
    GPUTextureCache cache; cache.Remove(path);
}
}
