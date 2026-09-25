#include "TextureStreamer.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <vector>

namespace NeoEngine
{

namespace
{

bool ReadToken(std::istream& stream, std::string& token)
{
    token.clear();
    char ch = 0;
    while (stream.get(ch)) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            continue;
        }
        if (ch == '#') {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        token.push_back(ch);
        break;
    }
    while (stream.get(ch)) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            return true;
        }
        if (ch == '#') {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return true;
        }
        token.push_back(ch);
    }
    return !token.empty();
}

bool LoadPpmRGBA(const std::string& path, uint32_t& width, uint32_t& height, std::vector<unsigned char>& pixels)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    std::string magic;
    std::string widthToken;
    std::string heightToken;
    std::string maxToken;
    if (!ReadToken(file, magic) || magic != "P6" ||
        !ReadToken(file, widthToken) || !ReadToken(file, heightToken) ||
        !ReadToken(file, maxToken)) {
        return false;
    }

    try {
        const unsigned long parsedWidth = std::stoul(widthToken);
        const unsigned long parsedHeight = std::stoul(heightToken);
        const unsigned long maxValue = std::stoul(maxToken);
        if (parsedWidth == 0 || parsedHeight == 0 ||
            parsedWidth > std::numeric_limits<uint32_t>::max() ||
            parsedHeight > std::numeric_limits<uint32_t>::max() ||
            maxValue != 255) {
            return false;
        }
        width = static_cast<uint32_t>(parsedWidth);
        height = static_cast<uint32_t>(parsedHeight);
    } catch (...) {
        return false;
    }

    const uint64_t pixelCount = static_cast<uint64_t>(width) * static_cast<uint64_t>(height);
    if (pixelCount > std::numeric_limits<std::size_t>::max() / 4U ||
        pixelCount > std::numeric_limits<std::size_t>::max() / 3U) {
        return false;
    }

    std::vector<unsigned char> rgb(static_cast<std::size_t>(pixelCount) * 3U);
    file.read(reinterpret_cast<char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
    if (file.gcount() != static_cast<std::streamsize>(rgb.size())) {
        return false;
    }

    pixels.resize(static_cast<std::size_t>(pixelCount) * 4U);
    for (std::size_t i = 0; i < static_cast<std::size_t>(pixelCount); ++i) {
        pixels[i * 4U + 0U] = rgb[i * 3U + 0U];
        pixels[i * 4U + 1U] = rgb[i * 3U + 1U];
        pixels[i * 4U + 2U] = rgb[i * 3U + 2U];
        pixels[i * 4U + 3U] = 255U;
    }
    return true;
}

}

TextureStreamer::~TextureStreamer()
{
    Destroy();
}

bool TextureStreamer::Initialize(VkDevice device,
                                 VkPhysicalDevice physicalDevice,
                                 VkQueue graphicsQueue,
                                 uint32_t graphicsQueueFamilyIndex)
{
    if (device == VK_NULL_HANDLE ||
        physicalDevice == VK_NULL_HANDLE ||
        graphicsQueue == VK_NULL_HANDLE) {
        return false;
    }

    Destroy();
    device_ = device;
    physicalDevice_ = physicalDevice;
    graphicsQueue_ = graphicsQueue;
    graphicsQueueFamilyIndex_ = graphicsQueueFamilyIndex;
    initialized_ = true;
    return true;
}

bool TextureStreamer::StreamIn(const std::string& key,
                               uint32_t width,
                               uint32_t height,
                               const void* pixels,
                               std::size_t byteCount)
{
    if (!initialized_ || key.empty() || width == 0 || height == 0 || pixels == nullptr || byteCount == 0) {
        return false;
    }

    const uint64_t requiredBytes = static_cast<uint64_t>(width) * static_cast<uint64_t>(height) * 4ULL;
    if (requiredBytes > std::numeric_limits<std::size_t>::max() ||
        byteCount < static_cast<std::size_t>(requiredBytes)) {
        return false;
    }

    VulkanGPUTexture texture;
    if (!texture.Initialize(device_, physicalDevice_, width, height, VK_FORMAT_R8G8B8A8_UNORM) ||
        !texture.UploadPixels(graphicsQueue_, graphicsQueueFamilyIndex_, physicalDevice_, pixels,
                              static_cast<VkDeviceSize>(requiredBytes)) ||
        !texture.CreateSampler()) {
        texture.Destroy();
        return false;
    }

    auto [it, inserted] = textures_.try_emplace(key, std::move(texture));
    if (!inserted) {
        it->second.Destroy();
        it->second = std::move(texture);
    }
    return true;
}

bool TextureStreamer::StreamIn(const std::string& path)
{
    if (!initialized_ || path.empty()) {
        return false;
    }

    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<unsigned char> pixels;
    if (!LoadPpmRGBA(path, width, height, pixels)) {
        return false;
    }
    return StreamIn(path, width, height, pixels.data(), pixels.size());
}

bool TextureStreamer::StreamOut(const std::string& key)
{
    if (!initialized_ || key.empty()) {
        return false;
    }
    return textures_.erase(key) != 0;
}

bool TextureStreamer::Contains(const std::string& key) const
{
    return initialized_ && textures_.find(key) != textures_.end();
}

VulkanGPUTexture* TextureStreamer::Find(const std::string& key)
{
    if (!initialized_) {
        return nullptr;
    }
    const auto it = textures_.find(key);
    return it == textures_.end() ? nullptr : &it->second;
}

const VulkanGPUTexture* TextureStreamer::Find(const std::string& key) const
{
    if (!initialized_) {
        return nullptr;
    }
    const auto it = textures_.find(key);
    return it == textures_.end() ? nullptr : &it->second;
}

void TextureStreamer::Destroy()
{
    textures_.clear();
    device_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
    graphicsQueue_ = VK_NULL_HANDLE;
    graphicsQueueFamilyIndex_ = 0;
    initialized_ = false;
}

}
