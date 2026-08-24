#pragma once

#include <unordered_map>
#include <string>
#include <cstdint>

namespace NeoEngine
{

struct GPUTexture
{
    uint32_t id;
    [[maybe_unused]] int width;
    [[maybe_unused]] int height;
};

class GPUTextureCache
{
public:

    GPUTexture LoadTexture(const std::string& path);

    void Remove(const std::string& path);

private:

    std::unordered_map<std::string, GPUTexture> cache;

};

}
