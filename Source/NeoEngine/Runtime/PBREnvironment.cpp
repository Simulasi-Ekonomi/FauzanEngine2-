#include "Runtime/PBREnvironment.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>
#include <utility>

namespace NeoEngine {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr uint32_t kFaces = 6;
constexpr VkDeviceSize kMaxUploadBytes = 256ULL * 1024ULL * 1024ULL;

uint32_t MipCount(uint32_t size) {
    uint32_t count = 1;
    while (size > 1) { size >>= 1; ++count; }
    return count;
}

uint16_t Half(float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    const uint32_t sign = (bits >> 16U) & 0x8000U;
    const uint32_t mantissa = bits & 0x007fffffU;
    const int32_t exponent = static_cast<int32_t>((bits >> 23U) & 0xffU) - 127;
    if (exponent <= -15) {
        if (exponent < -24) return static_cast<uint16_t>(sign);
        const uint32_t m = mantissa | 0x00800000U;
        return static_cast<uint16_t>(sign | (m >> static_cast<uint32_t>(-exponent - 1 + 13)));
    }
    if (exponent >= 16) return static_cast<uint16_t>(sign | 0x7c00U);
    return static_cast<uint16_t>(sign | (static_cast<uint32_t>(exponent + 15) << 10U) | (mantissa >> 13U));
}

void AppendRGBA16F(std::vector<uint8_t>& dst, const glm::vec4& value) {
    const uint16_t half[4] = {Half(value.x), Half(value.y), Half(value.z), Half(value.w)};
    const size_t old = dst.size();
    dst.resize(old + sizeof(half));
    std::memcpy(dst.data() + old, half, sizeof(half));
}

uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeBits, VkMemoryPropertyFlags required) {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        if ((typeBits & (1U << i)) != 0 && (properties.memoryTypes[i].propertyFlags & required) == required) return i;
    }
    return UINT32_MAX;
}

bool CreateImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t size, uint32_t mipLevels,
                 VkImage& image, VkDeviceMemory& memory) {
    image = VK_NULL_HANDLE;
    memory = VK_NULL_HANDLE;
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    info.extent = {size, size, 1};
    info.mipLevels = mipLevels;
    info.arrayLayers = kFaces;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(device, &info, nullptr, &image) != VK_SUCCESS) return false;

    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device, image, &requirements);
    const uint32_t memoryType = FindMemoryType(physicalDevice, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memoryType == UINT32_MAX) {
        vkDestroyImage(device, image, nullptr);
        image = VK_NULL_HANDLE;
        return false;
    }

    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memoryType;
    if (vkAllocateMemory(device, &allocation, nullptr, &memory) != VK_SUCCESS ||
        vkBindImageMemory(device, image, memory, 0) != VK_SUCCESS) {
        if (memory != VK_NULL_HANDLE) vkFreeMemory(device, memory, nullptr);
        vkDestroyImage(device, image, nullptr);
        image = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

bool CreateCubeViewAndSampler(VkDevice device, VkImage image, uint32_t mipLevels,
                              VkImageView& view, VkSampler& sampler) {
    view = VK_NULL_HANDLE;
    sampler = VK_NULL_HANDLE;
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = kFaces;
    if (vkCreateImageView(device, &viewInfo, nullptr, &view) != VK_SUCCESS) return false;

    VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels - 1);
    samplerInfo.maxAnisotropy = 1.0f;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        vkDestroyImageView(device, view, nullptr);
        view = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

glm::vec3 FaceDirection(uint32_t face, float u, float v) {
    const glm::vec3 d[] = {{1,-v,-u},{-1,-v,u},{u,1,v},{u,-1,-v},{u,-v,1},{-u,-v,-1}};
    return glm::normalize(d[face]);
}

glm::vec3 SampleEquirect(const std::vector<glm::vec3>& image, uint32_t width, uint32_t height,
                         const glm::vec3& direction) {
    const float phi = std::atan2(direction.z, direction.x);
    const float theta = std::asin(std::clamp(direction.y, -1.0f, 1.0f));
    const float x = (phi / (2.0f * kPi) + 0.5f) * static_cast<float>(width);
    const float y = (0.5f - theta / kPi) * static_cast<float>(height);
    const int ix = static_cast<int>(std::floor(x));
    const int iy = static_cast<int>(std::floor(y));
    const int x0 = ((ix % static_cast<int>(width)) + static_cast<int>(width)) % static_cast<int>(width);
    const int x1 = (x0 + 1) % static_cast<int>(width);
    const int y0 = std::clamp(iy, 0, static_cast<int>(height) - 1);
    const int y1 = std::min(y0 + 1, static_cast<int>(height) - 1);
    const float tx = x - std::floor(x), ty = y - std::floor(y);
    const auto at = [&](int px, int py) { return image[static_cast<size_t>(py) * width + static_cast<size_t>(px)]; };
    return glm::mix(glm::mix(at(x0, y0), at(x1, y0), tx), glm::mix(at(x0, y1), at(x1, y1), tx), ty);
}

bool DecodeRadiance(const std::string& path, std::vector<glm::vec3>& pixels, uint32_t& width, uint32_t& height) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::string line;
    bool formatOk = false;
    while (std::getline(file, line) && !line.empty()) {
        if (line.find("FORMAT=32-bit_rle_rgbe") != std::string::npos) formatOk = true;
    }
    if (!formatOk || !std::getline(file, line)) return false;

    int h = 0, w = 0;
    char ys = 0, xs = 0;
    if (std::sscanf(line.c_str(), "%cY %d %cX %d", &ys, &h, &xs, &w) != 4 ||
        h <= 0 || w <= 0 || ys != '-' || xs != '+') return false;
    width = static_cast<uint32_t>(w);
    height = static_cast<uint32_t>(h);
    if (static_cast<uint64_t>(width) * height > 16ULL * 1024ULL * 1024ULL) return false;
    pixels.resize(static_cast<size_t>(width) * height);

    std::vector<uint8_t> channel(static_cast<size_t>(width) * 4U);
    for (uint32_t y = 0; y < height; ++y) {
        uint8_t header[4]{};
        file.read(reinterpret_cast<char*>(header), 4);
        if (!file || header[0] != 2 || header[1] != 2 || (header[2] & 0x80U) != 0) return false;
        const uint32_t scanWidth = (static_cast<uint32_t>(header[2]) << 8U) | header[3];
        if (scanWidth != width) return false;
        for (uint32_t c = 0; c < 4; ++c) {
            uint32_t x = 0;
            while (x < width) {
                uint8_t a = 0, b = 0;
                file.read(reinterpret_cast<char*>(&a), 1);
                file.read(reinterpret_cast<char*>(&b), 1);
                if (!file) return false;
                if (a > 128U) {
                    const uint32_t count = a - 128U;
                    if (count == 0 || x + count > width) return false;
                    std::fill(channel.begin() + static_cast<size_t>(c) * width + x,
                              channel.begin() + static_cast<size_t>(c) * width + x + count, b);
                    x += count;
                } else {
                    if (x + 1 >= width) return false;
                    channel[static_cast<size_t>(c) * width + x++] = a;
                    channel[static_cast<size_t>(c) * width + x++] = b;
                }
            }
        }
        for (uint32_t x = 0; x < width; ++x) {
            const uint8_t r = channel[x], g = channel[width + x], b = channel[2 * width + x], e = channel[3 * width + x];
            pixels[static_cast<size_t>(y) * width + x] =
                e == 0 ? glm::vec3(0.0f) : glm::vec3(r, g, b) * std::ldexp(1.0f, static_cast<int>(e) - 136);
        }
    }
    return true;
}

void BuildFace(const std::vector<glm::vec3>& source, uint32_t sw, uint32_t sh, uint32_t size,
               uint32_t face, std::vector<glm::vec4>& out) {
    const size_t base = static_cast<size_t>(face) * size * size;
    for (uint32_t y = 0; y < size; ++y) for (uint32_t x = 0; x < size; ++x) {
        const float u = 2.0f * (static_cast<float>(x) + 0.5f) / size - 1.0f;
        const float v = 2.0f * (static_cast<float>(y) + 0.5f) / size - 1.0f;
        out[base + static_cast<size_t>(y) * size + x] =
            glm::vec4(SampleEquirect(source, sw, sh, FaceDirection(face, u, v)), 1.0f);
    }
}

void BuildIrradiance(const std::vector<glm::vec3>& source, uint32_t sw, uint32_t sh,
                     uint32_t outSize, std::vector<glm::vec4>& out) {
    out.resize(static_cast<size_t>(kFaces) * outSize * outSize);
    constexpr uint32_t samples = 64;
    for (uint32_t face = 0; face < kFaces; ++face) for (uint32_t y = 0; y < outSize; ++y) for (uint32_t x = 0; x < outSize; ++x) {
        const float u = 2.0f * (static_cast<float>(x) + 0.5f) / outSize - 1.0f;
        const float v = 2.0f * (static_cast<float>(y) + 0.5f) / outSize - 1.0f;
        const glm::vec3 n = FaceDirection(face, u, v);
        const glm::vec3 up = std::abs(n.y) < 0.99f ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
        const glm::vec3 tangent = glm::normalize(glm::cross(up, n));
        const glm::vec3 bitangent = glm::cross(n, tangent);
        glm::vec3 sum(0.0f);
        for (uint32_t i = 0; i < samples; ++i) {
            const float a = (static_cast<float>(i) + 0.5f) / samples;
            const float phi = 2.0f * kPi * std::fmod(static_cast<float>(i) * 0.61803398875f, 1.0f);
            const float z = std::sqrt(1.0f - a);
            const float r = std::sqrt(a);
            const glm::vec3 d = glm::normalize(tangent * (r * std::cos(phi)) + bitangent * (r * std::sin(phi)) + n * z);
            sum += SampleEquirect(source, sw, sh, d) * std::max(glm::dot(n, d), 0.0f);
        }
        out[static_cast<size_t>(face) * outSize * outSize + static_cast<size_t>(y) * outSize + x] =
            glm::vec4(sum * (kPi / samples), 1.0f);
    }
}

void BuildPrefilter(const std::vector<glm::vec3>& source, uint32_t sw, uint32_t sh,
                    uint32_t faceSize, uint32_t samples, uint32_t mip, std::vector<glm::vec4>& out) {
    const uint32_t size = std::max(1U, faceSize >> mip);
    const uint32_t mipCount = MipCount(faceSize);
    const float roughness = mipCount > 1 ? static_cast<float>(mip) / static_cast<float>(mipCount - 1) : 0.0f;
    out.resize(static_cast<size_t>(kFaces) * size * size);
    for (uint32_t face = 0; face < kFaces; ++face) for (uint32_t y = 0; y < size; ++y) for (uint32_t x = 0; x < size; ++x) {
        const float u = 2.0f * (static_cast<float>(x) + 0.5f) / size - 1.0f;
        const float v = 2.0f * (static_cast<float>(y) + 0.5f) / size - 1.0f;
        const glm::vec3 n = FaceDirection(face, u, v);
        const glm::vec3 up = std::abs(n.y) < 0.99f ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
        const glm::vec3 tangent = glm::normalize(glm::cross(up, n));
        const glm::vec3 bitangent = glm::cross(n, tangent);
        glm::vec3 sum(0.0f);
        float weight = 0.0f;
        for (uint32_t i = 0; i < samples; ++i) {
            const float t = (static_cast<float>(i) + 0.5f) / samples;
            const float phi = 2.0f * kPi * std::fmod(static_cast<float>(i) * 0.754877666f, 1.0f);
            const float z = std::pow(1.0f - t, 1.0f / (roughness * roughness + 0.001f));
            const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
            const glm::vec3 d = glm::normalize(tangent * (r * std::cos(phi)) + bitangent * (r * std::sin(phi)) + n * z);
            const float w = std::max(glm::dot(n, d), 0.0f);
            sum += SampleEquirect(source, sw, sh, d) * w;
            weight += w;
        }
        out[static_cast<size_t>(face) * size * size + static_cast<size_t>(y) * size + x] =
            glm::vec4(weight > 0.0f ? sum / weight : glm::vec3(0.0f), 1.0f);
    }
}

bool UploadImage(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, uint32_t queueFamily,
                 uint32_t baseSize, uint32_t mipLevels,
                 const std::vector<std::vector<glm::vec4>>& mips,
                 PBREnvironment::ImageResource& resource) {
    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(physicalDevice, VK_FORMAT_R16G16B16A16_SFLOAT, &props);
    const VkFormatFeatureFlags required = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    if ((props.optimalTilingFeatures & required) != required || mips.size() != mipLevels) return false;

    std::vector<uint8_t> bytes;
    std::vector<VkBufferImageCopy> regions;
    for (uint32_t mip = 0; mip < mipLevels; ++mip) {
        const uint32_t size = std::max(1U, baseSize >> mip);
        const VkDeviceSize offset = bytes.size();
        for (const glm::vec4& pixel : mips[mip]) AppendRGBA16F(bytes, pixel);
        const VkDeviceSize expected = static_cast<VkDeviceSize>(kFaces) * size * size * sizeof(uint16_t) * 4ULL;
        if (bytes.size() - offset != expected) return false;
        VkBufferImageCopy copy{};
        copy.bufferOffset = offset;
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.mipLevel = mip;
        copy.imageSubresource.baseArrayLayer = 0;
        copy.imageSubresource.layerCount = kFaces;
        copy.imageExtent = {size, size, 1};
        regions.push_back(copy);
    }
    if (bytes.empty() || bytes.size() > kMaxUploadBytes) return false;

    if (!CreateImage(physicalDevice, device, baseSize, mipLevels, resource.image, resource.memory)) return false;

    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    bool success = false;

    do {
        VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bi.size = bytes.size(); bi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device, &bi, nullptr, &buffer) != VK_SUCCESS) break;
        VkMemoryRequirements br{}; vkGetBufferMemoryRequirements(device, buffer, &br);
        const uint32_t mt = FindMemoryType(physicalDevice, br.memoryTypeBits,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (mt == UINT32_MAX) break;
        VkMemoryAllocateInfo ba{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; ba.allocationSize = br.size; ba.memoryTypeIndex = mt;
        if (vkAllocateMemory(device, &ba, nullptr, &bufferMemory) != VK_SUCCESS) break;
        if (vkBindBufferMemory(device, buffer, bufferMemory, 0) != VK_SUCCESS) break;
        void* mapped = nullptr;
        if (vkMapMemory(device, bufferMemory, 0, bytes.size(), 0, &mapped) != VK_SUCCESS) break;
        std::memcpy(mapped, bytes.data(), bytes.size());
        vkUnmapMemory(device, bufferMemory);

        VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pi.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; pi.queueFamilyIndex = queueFamily;
        if (vkCreateCommandPool(device, &pi, nullptr, &pool) != VK_SUCCESS) break;
        VkCommandBuffer cb = VK_NULL_HANDLE;
        VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cai.commandPool = pool; cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cai.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(device, &cai, &cb) != VK_SUCCESS) break;
        VkCommandBufferBeginInfo cbi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        cbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (vkBeginCommandBuffer(cb, &cbi) != VK_SUCCESS) break;

        VkImageMemoryBarrier toDst{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        toDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toDst.image = resource.image;
        toDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        toDst.subresourceRange.baseMipLevel = 0;
        toDst.subresourceRange.levelCount = mipLevels;
        toDst.subresourceRange.baseArrayLayer = 0;
        toDst.subresourceRange.layerCount = kFaces;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &toDst);
        vkCmdCopyBufferToImage(cb, buffer, resource.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               static_cast<uint32_t>(regions.size()), regions.data());

        VkImageMemoryBarrier toShader{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        toShader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toShader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        toShader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toShader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        toShader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toShader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toShader.image = resource.image;
        toShader.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        toShader.subresourceRange.baseMipLevel = 0;
        toShader.subresourceRange.levelCount = mipLevels;
        toShader.subresourceRange.baseArrayLayer = 0;
        toShader.subresourceRange.layerCount = kFaces;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &toShader);
        if (vkEndCommandBuffer(cb) != VK_SUCCESS) break;

        VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        if (vkCreateFence(device, &fi, nullptr, &fence) != VK_SUCCESS) break;
        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO}; si.commandBufferCount = 1; si.pCommandBuffers = &cb;
        if (vkQueueSubmit(queue, 1, &si, fence) != VK_SUCCESS ||
            vkWaitForFences(device, 1, &fence, VK_TRUE, 5'000'000'000ULL) != VK_SUCCESS) break;
        success = true;
    } while (false);

    if (fence) vkDestroyFence(device, fence, nullptr);
    if (pool) vkDestroyCommandPool(device, pool, nullptr);
    if (buffer) vkDestroyBuffer(device, buffer, nullptr);
    if (bufferMemory) vkFreeMemory(device, bufferMemory, nullptr);
    if (!success) return false;
    return CreateCubeViewAndSampler(device, resource.image, mipLevels, resource.view, resource.sampler);
}

} // namespace

PBREnvironment::~PBREnvironment() { Destroy(); }
PBREnvironment::PBREnvironment(PBREnvironment&& other) noexcept { *this = std::move(other); }

PBREnvironment& PBREnvironment::operator=(PBREnvironment&& other) noexcept {
    if (this == &other) return *this;
    Destroy();
    config_ = other.config_; settings_ = other.settings_;
    sourceWidth_ = other.sourceWidth_; sourceHeight_ = other.sourceHeight_; prefilterMipLevels_ = other.prefilterMipLevels_;
    source_ = std::move(other.source_); environment_ = std::move(other.environment_);
    irradiance_ = std::move(other.irradiance_); prefilteredMips_ = std::move(other.prefilteredMips_);
    context_ = std::move(other.context_);
    device_ = other.device_; physicalDevice_ = other.physicalDevice_; graphicsQueue_ = other.graphicsQueue_; graphicsQueueFamily_ = other.graphicsQueueFamily_;
    environmentResource_ = other.environmentResource_; irradianceResource_ = other.irradianceResource_; prefilteredResource_ = other.prefilteredResource_;
    other.device_ = VK_NULL_HANDLE; other.physicalDevice_ = VK_NULL_HANDLE; other.graphicsQueue_ = VK_NULL_HANDLE; other.graphicsQueueFamily_ = UINT32_MAX;
    other.environmentResource_ = {}; other.irradianceResource_ = {}; other.prefilteredResource_ = {};
    return *this;
}

bool PBREnvironment::LoadHDR(const std::string& path, const PBREnvironmentConfig& config) {
    Destroy();
    if (config.environmentFaceSize == 0 || config.irradianceFaceSize == 0 || config.prefilterFaceSize == 0 || config.prefilterSamples == 0 ||
        !std::isfinite(config.environmentIntensity) || !std::isfinite(config.irradianceStrength) ||
        config.environmentIntensity < 0.0f || config.irradianceStrength < 0.0f) return false;
    config_ = config;
    settings_ = {config.environmentIntensity, static_cast<float>(MipCount(config.prefilterFaceSize) - 1), config.irradianceStrength};
    if (!ValidatePBRIBLSettings(settings_)) return false;
    if (!DecodeRadiance(path, source_, sourceWidth_, sourceHeight_)) return false;
    environment_.resize(static_cast<size_t>(kFaces) * config_.environmentFaceSize * config_.environmentFaceSize);
    for (uint32_t face = 0; face < kFaces; ++face) BuildFace(source_, sourceWidth_, sourceHeight_, config_.environmentFaceSize, face, environment_);
    BuildIrradiance(source_, sourceWidth_, sourceHeight_, config_.irradianceFaceSize, irradiance_);
    prefilterMipLevels_ = MipCount(config_.prefilterFaceSize);
    prefilteredMips_.resize(prefilterMipLevels_);
    for (uint32_t mip = 0; mip < prefilterMipLevels_; ++mip)
        BuildPrefilter(source_, sourceWidth_, sourceHeight_, config_.prefilterFaceSize, config_.prefilterSamples, mip, prefilteredMips_[mip]);
    return true;
}

bool PBREnvironment::UploadToVulkan() {
    if (!IsCpuReady()) return false;
    if (IsGpuReady()) return true;
    context_ = std::make_unique<VulkanContext>();
    if (!context_->Initialize()) { context_.reset(); return false; }
    device_ = context_->Device(); physicalDevice_ = context_->PhysicalDevice();
    graphicsQueue_ = context_->GraphicsQueue(); graphicsQueueFamily_ = context_->GraphicsQueueFamily();

    const std::vector<std::vector<glm::vec4>> environmentMips{environment_};
    const std::vector<std::vector<glm::vec4>> irradianceMips{irradiance_};
    if (!UploadImage(physicalDevice_, device_, graphicsQueue_, graphicsQueueFamily_, config_.environmentFaceSize, 1, environmentMips, environmentResource_) ||
        !UploadImage(physicalDevice_, device_, graphicsQueue_, graphicsQueueFamily_, config_.irradianceFaceSize, 1, irradianceMips, irradianceResource_) ||
        !UploadImage(physicalDevice_, device_, graphicsQueue_, graphicsQueueFamily_, config_.prefilterFaceSize, prefilterMipLevels_, prefilteredMips_, prefilteredResource_)) {
        Destroy();
        return false;
    }
    return IsGpuReady();
}

void PBREnvironment::Destroy() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        auto destroy = [&](ImageResource& resource) {
            if (resource.sampler) vkDestroySampler(device_, resource.sampler, nullptr);
            if (resource.view) vkDestroyImageView(device_, resource.view, nullptr);
            if (resource.image) vkDestroyImage(device_, resource.image, nullptr);
            if (resource.memory) vkFreeMemory(device_, resource.memory, nullptr);
            resource = {};
        };
        destroy(environmentResource_); destroy(irradianceResource_); destroy(prefilteredResource_);
    }
    environmentResource_ = {}; irradianceResource_ = {}; prefilteredResource_ = {};
    device_ = VK_NULL_HANDLE; physicalDevice_ = VK_NULL_HANDLE; graphicsQueue_ = VK_NULL_HANDLE; graphicsQueueFamily_ = UINT32_MAX;
    context_.reset();
    source_.clear(); environment_.clear(); irradiance_.clear(); prefilteredMips_.clear();
    sourceWidth_ = sourceHeight_ = prefilterMipLevels_ = 0;
}

} // namespace NeoEngine
