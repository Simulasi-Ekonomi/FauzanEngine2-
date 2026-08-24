#include <cassert>
#include "GPUTextureCache.h"

namespace NeoEngine
{

GPUTexture GPUTextureCache::LoadTexture(const std::string& path)
{
    if(cache.count(path))
        return cache[path];

    GPUTexture tex;

    tex.id = cache.size() + 1;
    tex.width = 0;
    tex.height = 0;

    cache[path] = tex;

    return tex;
}

void GPUTextureCache::Remove(const std::string& path)
{
    cache.erase(path);
}

}
