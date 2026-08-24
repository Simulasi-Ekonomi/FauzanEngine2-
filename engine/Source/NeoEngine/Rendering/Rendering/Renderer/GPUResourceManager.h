#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>

struct GPUBufferHandle
{
    uint32_t vao;
    uint32_t vbo;
    uint32_t ebo;
};

struct TextureHandle
{
    uint32_t id;
};

class GPUResourceManager
{
public:

    GPUResourceManager();

    GPUBufferHandle CreateMesh(
        const std::vector<float>& vertices,
        const std::vector<uint32_t>& indices
    );

    TextureHandle CreateTexture(const std::string& path);

    void DestroyMesh(const std::string& name);

    GPUBufferHandle GetMesh(const std::string& name);

private:

    std::unordered_map<std::string, GPUBufferHandle> meshPool;
    std::unordered_map<std::string, TextureHandle> texturePool;
};
