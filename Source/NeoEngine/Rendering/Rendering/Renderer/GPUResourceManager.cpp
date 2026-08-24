#include <cassert>
#include "GPUResourceManager.h"

GPUResourceManager::GPUResourceManager()
{
}

GPUBufferHandle GPUResourceManager::CreateMesh(
    const std::vector<float>& vertices,
    const std::vector<uint32_t>& indices
)
{
    GPUBufferHandle handle;

    handle.vao = 0;
    handle.vbo = 0;
    handle.ebo = 0;

    return handle;
}

TextureHandle GPUResourceManager::CreateTexture(const std::string& path)
{
    TextureHandle t;
    t.id = 0;

    texturePool[path] = t;

    return t;
}

void GPUResourceManager::DestroyMesh(const std::string& name)
{
    if(meshPool.find(name) != meshPool.end())
        meshPool.erase(name);
}

GPUBufferHandle GPUResourceManager::GetMesh(const std::string& name)
{
    return meshPool[name];
}
