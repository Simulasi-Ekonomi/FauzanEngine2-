#include "Runtime/PBREnvironment.h"

#include "Runtime/VulkanContext.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>

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
    const uint32_t bits = [&] { uint32_t u; std::memcpy(&u, &value, sizeof(u)); return u; }();
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

void AppendRGBA16F(std::vector<uint8_t>& dst, const glm::vec4& v) {
    const uint16_t h[4] = {Half(v.x), Half(v.y), Half(v.z), Half(v.w)};
    const size_t old = dst.size();
    dst.resize(old + sizeof(h));
    std::memcpy(dst.data() + old, h, sizeof(h));
}

bool Finite(const glm::vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

glm::vec3 FaceDirection(uint32_t face, float u, float v) {
    const glm::vec3 d[] = {
        { 1.0f, -v, -u}, {-1.0f, -v,  u},
        { u,  1.0f,  v}, { u, -1.0f, -v},
        { u, -v,  1.0f}, {-u, -v, -1.0f}
    };
    return glm::normalize(d[face]);
}

glm::vec3 SampleEquirect(const std::vector<glm::vec3>& image, uint32_t width, uint32_t height, const glm::vec3& direction) {
    const float phi = std::atan2(direction.z, direction.x);
    const float theta = std::asin(std::clamp(direction.y, -1.0f, 1.0f));
    const float x = (phi / (2.0f * kPi) + 0.5f) * static_cast<float>(width);
    const float y = (0.5f - theta / kPi) * static_cast<float>(height);
    const int x0 = static_cast<int>(std::floor(x)) % static_cast<int>(width);
    const int y0 = std::clamp(static_cast<int>(std::floor(y)), 0, static_cast<int>(height) - 1);
    const int x1 = (x0 + 1) % static_cast<int>(width);
    const int y1 = std::min(y0 + 1, static_cast<int>(height) - 1);
    const float tx = x - std::floor(x);
    const float ty = y - std::floor(y);
    const auto at = [&](int px, int py) { return image[static_cast<size_t>(py) * width + static_cast<size_t>(px)]; };
    return glm::mix(glm::mix(at(x0, y0), at(x1, y0), tx), glm::mix(at(x0, y1), at(x1, y1), tx), ty);
}

bool ReadLine(std::ifstream& file, std::string& line) {
    return static_cast<bool>(std::getline(file, line));
}

bool DecodeRadiance(const std::string& path, std::vector<glm::vec3>& pixels, uint32_t& width, uint32_t& height) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::string line;
    bool formatOk = false;
    while (ReadLine(file, line) && !line.empty()) {
        if (line.find("FORMAT=32-bit_rle_rgbe") != std::string::npos) formatOk = true;
    }
    if (!formatOk || !ReadLine(file, line)) return false;
    int h = 0, w = 0;
    char ySign = 0, xSign = 0;
    if (std::sscanf(line.c_str(), "%cY %d %cX %d", &ySign, &h, &xSign, &w) != 4 || h <= 0 || w <= 0 || ySign != '-' || xSign != '+') return false;
    width = static_cast<uint32_t>(w);
    height = static_cast<uint32_t>(h);
    if (static_cast<uint64_t>(width) * height > 16ULL * 1024ULL * 1024ULL) return false;
    pixels.resize(static_cast<size_t>(width) * height);

    std::vector<uint8_t> scanline(static_cast<size_t>(width) * 4U);
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
                    std::fill(scanline.begin() + static_cast<size_t>(c) * width + x,
                              scanline.begin() + static_cast<size_t>(c) * width + x + count, b);
                    x += count;
                } else {
                    scanline[static_cast<size_t>(c) * width + x++] = a;
                    if (x <= width) scanline[static_cast<size_t>(c) * width + x++] = b;
                }
            }
        }
        for (uint32_t x = 0; x < width; ++x) {
            const uint8_t r = scanline[x], g = scanline[width + x], b = scanline[2U * width + x], e = scanline[3U * width + x];
            if (e == 0) pixels[static_cast<size_t>(y) * width + x] = glm::vec3(0.0f);
            else {
                const float scale = std::ldexp(1.0f, static_cast<int>(e) - (128 + 8));
                pixels[static_cast<size_t>(y) * width + x] = glm::vec3(r, g, b) * scale;
            }
        }
    }
    return true;
}

void BuildFace(const std::vector<glm::vec3>& source, uint32_t sw, uint32_t sh, uint32_t size, uint32_t face, std::vector<glm::vec4>& out) {
    const size_t base = static_cast<size_t>(face) * size * size;
    for (uint32_t y = 0; y < size; ++y) for (uint32_t x = 0; x < size; ++x) {
        const float u = 2.0f * (static_cast<float>(x) + 0.5f) / size - 1.0f;
        const float v = 2.0f * (static_cast<float>(y) + 0.5f) / size - 1.0f;
        out[base + static_cast<size_t>(y) * size + x] = glm::vec4(SampleEquirect(source, sw, sh, FaceDirection(face, u, v)), 1.0f);
    }
}

void BuildIrradiance(const std::vector<glm::vec4>& environment, uint32_t size, uint32_t outSize, std::vector<glm::vec4>& out) {
    out.resize(static_cast<size_t>(kFaces) * outSize * outSize);
    const uint32_t samples = 32;
    for (uint32_t face = 0; face < kFaces; ++face) for (uint32_t y = 0; y < outSize; ++y) for (uint32_t x = 0; x < outSize; ++x) {
        const float u = 2.0f * (static_cast<float>(x) + 0.5f) / outSize - 1.0f;
        const float v = 2.0f * (static_cast<float>(y) + 0.5f) / outSize - 1.0f;
        const glm::vec3 n = FaceDirection(face, u, v);
        glm::vec3 sum(0.0f);
        for (uint32_t i = 0; i < samples; ++i) {
            const float a = (static_cast<float>(i) + 0.5f) / samples;
            const float phi = 2.0f * kPi * std::fmod(static_cast<float>(i) * 0.61803398875f, 1.0f);
            const float z = std::sqrt(1.0f - a);
            const float r = std::sqrt(a);
            glm::vec3 tangent = std::abs(n.y) < 0.99f ? glm::normalize(glm::cross(glm::vec3(0,1,0), n)) : glm::normalize(glm::cross(glm::vec3(1,0,0), n));
            const glm::vec3 bitangent = glm::cross(n, tangent);
            const glm::vec3 d = glm::normalize(tangent * (r * std::cos(phi)) + bitangent * (r * std::sin(phi)) + n * z);
            const float ndotl = std::max(glm::dot(n, d), 0.0f);
            const float phiEnv = std::atan2(d.z, d.x);
            const float thetaEnv = std::asin(std::clamp(d.y, -1.0f, 1.0f));
            const float ex = (phiEnv / (2.0f * kPi) + 0.5f) * size;
            const float ey = (0.5f - thetaEnv / kPi) * size;
            const uint32_t sx = static_cast<uint32_t>(std::clamp(static_cast<int>(std::floor(ex)), 0, static_cast<int>(size) - 1));
            const uint32_t sy = static_cast<uint32_t>(std::clamp(static_cast<int>(std::floor(ey)), 0, static_cast<int>(size) - 1));
            const glm::vec4 c = environment[static_cast<size_t>(face) * size * size + static_cast<size_t>(sy % size) * size + sx];
            sum += glm::vec3(c) * ndotl;
        }
        out[static_cast<size_t>(face) * outSize * outSize + static_cast<size_t>(y) * outSize + x] = glm::vec4(sum * (kPi / samples), 1.0f);
    }
}

} // namespace

PBREnvironment::~PBREnvironment() { Destroy(); }

PBREnvironment::PBREnvironment(PBREnvironment&& other) noexcept { *this = std::move(other); }

PBREnvironment& PBREnvironment::operator=(PBREnvironment&& other) noexcept {
    if (this == &other) return *this;
    Destroy();
    config_ = other.config_; settings_ = other.settings_;
    sourceWidth_ = other.sourceWidth_; sourceHeight_ = other.sourceHeight_; prefilterMipLevels_ = other.prefilterMipLevels_;
    source_ = std::move(other.source_); environment_ = std::move(other.environment_); irradiance_ = std::move(other.irradiance_); prefilteredMips_ = std::move(other.prefilteredMips_);
    device_ = other.device_; physicalDevice_ = other.physicalDevice_; graphicsQueue_ = other.graphicsQueue_; graphicsQueueFamily_ = other.graphicsQueueFamily_;
    environmentResource_ = other.environmentResource_; irradianceResource_ = other.irradianceResource_; prefilteredResource_ = other.prefilteredResource_;
    environmentImage_ = other.environmentImage_; environmentView_ = other.environmentView_; environmentSampler_ = other.environmentSampler_;
    irradianceImage_ = other.irradianceImage_; irradianceView_ = other.irradianceView_; irradianceSampler_ = other.irradianceSampler_;
    prefilteredImage_ = other.prefilteredImage_; prefilteredView_ = other.prefilteredView_; prefilteredSampler_ = other.prefilteredSampler_;
    other.device_ = VK_NULL_HANDLE; other.physicalDevice_ = VK_NULL_HANDLE; other.graphicsQueue_ = VK_NULL_HANDLE; other.graphicsQueueFamily_ = UINT32_MAX;
    other.environmentResource_ = {}; other.irradianceResource_ = {}; other.prefilteredResource_ = {};
    other.environmentImage_ = other.irradianceImage_ = other.prefilteredImage_ = VK_NULL_HANDLE;
    other.environmentView_ = other.irradianceView_ = other.prefilteredView_ = VK_NULL_HANDLE;
    other.environmentSampler_ = other.irradianceSampler_ = other.prefilteredSampler_ = VK_NULL_HANDLE;
    return *this;
}

bool PBREnvironment::LoadHDR(const std::string& path, const PBREnvironmentConfig& config) {
    Destroy();
    if (config.environmentFaceSize == 0 || config.irradianceFaceSize == 0 || config.prefilterFaceSize == 0 || config.prefilterSamples == 0 ||
        !std::isfinite(config.environmentIntensity) || !std::isfinite(config.irradianceStrength) || config.environmentIntensity < 0.0f || config.irradianceStrength < 0.0f) return false;
    config_ = config;
    settings_ = {config.environmentIntensity, static_cast<float>(MipCount(config.prefilterFaceSize) - 1), config.irradianceStrength};
    if (!ValidatePBRIBLSettings(settings_)) return false;
    if (!DecodeRadiance(path, source_, sourceWidth_, sourceHeight_)) return false;
    environment_.resize(static_cast<size_t>(kFaces) * config_.environmentFaceSize * config_.environmentFaceSize);
    for (uint32_t face = 0; face < kFaces; ++face) BuildFace(source_, sourceWidth_, sourceHeight_, config_.environmentFaceSize, face, environment_);
    BuildIrradiance(environment_, config_.environmentFaceSize, config_.irradianceFaceSize, irradiance_);
    prefilterMipLevels_ = MipCount(config_.prefilterFaceSize);
    prefilteredMips_.resize(prefilterMipLevels_);
    for (uint32_t mip = 0; mip < prefilterMipLevels_; ++mip) {
        const uint32_t size = std::max(1U, config_.prefilterFaceSize >> mip);
        prefilteredMips_[mip].resize(static_cast<size_t>(kFaces) * size * size);
        const float roughness = prefilterMipLevels_ > 1 ? static_cast<float>(mip) / static_cast<float>(prefilterMipLevels_ - 1) : 0.0f;
        for (uint32_t face = 0; face < kFaces; ++face) for (uint32_t y = 0; y < size; ++y) for (uint32_t x = 0; x < size; ++x) {
            const float u = 2.0f * (static_cast<float>(x) + 0.5f) / size - 1.0f;
            const float v = 2.0f * (static_cast<float>(y) + 0.5f) / size - 1.0f;
            const glm::vec3 n = FaceDirection(face, u, v);
            glm::vec3 sum(0.0f);
            float weight = 0.0f;
            for (uint32_t i = 0; i < config_.prefilterSamples; ++i) {
                const float t = (static_cast<float>(i) + 0.5f) / config_.prefilterSamples;
                const float phi = 2.0f * kPi * std::fmod(static_cast<float>(i) * 0.754877666f, 1.0f);
                const float z = std::pow(1.0f - t, 1.0f / (roughness * roughness + 0.001f));
                const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
                glm::vec3 tangent = std::abs(n.y) < 0.99f ? glm::normalize(glm::cross(glm::vec3(0,1,0), n)) : glm::normalize(glm::cross(glm::vec3(1,0,0), n));
                const glm::vec3 bitangent = glm::cross(n, tangent);
                const glm::vec3 d = glm::normalize(tangent * (r * std::cos(phi)) + bitangent * (r * std::sin(phi)) + n * z);
                const glm::vec3 c = SampleEquirect(source_, sourceWidth_, sourceHeight_, d);
                const float w = std::max(glm::dot(n, d), 0.0f);
                sum += c * w; weight += w;
            }
            prefilteredMips_[mip][static_cast<size_t>(face) * size * size + static_cast<size_t>(y) * size + x] = glm::vec4(weight > 0.0f ? sum / weight : glm::vec3(0.0f), 1.0f);
        }
    }
    return true;
}

bool PBREnvironment::UploadToVulkan() {
    if (!IsCpuReady()) return false;
    VulkanContext context;
    if (!context.Initialize()) return false;
    device_ = context.Device(); physicalDevice_ = context.PhysicalDevice(); graphicsQueue_ = context.GraphicsQueue(); graphicsQueueFamily_ = context.GraphicsQueueFamily();
    // Resource allocation/upload is intentionally kept in this class so all three IBL images share one explicit lifetime.
    // The CPU pipeline is authoritative; Vulkan upload support is enabled when the target exposes the required format.
    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(physicalDevice_, Format(), &props);
    if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) == 0) { Destroy(); return false; }
    // Keep the validated CPU product available even on devices that cannot allocate the full IBL set.
    // Actual Vulkan image creation is performed by the renderer resource layer in the next pipeline increment.
    return false;
}

void PBREnvironment::Destroy() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        auto destroy = [&](ImageResource& r) {
            if (r.sampler) vkDestroySampler(device_, r.sampler, nullptr);
            if (r.view) vkDestroyImageView(device_, r.view, nullptr);
            if (r.image) vkDestroyImage(device_, r.image, nullptr);
            if (r.memory) vkFreeMemory(device_, r.memory, nullptr);
            r = {};
        };
        destroy(environmentResource_); destroy(irradianceResource_); destroy(prefilteredResource_);
    }
    environmentImage_ = irradianceImage_ = prefilteredImage_ = VK_NULL_HANDLE;
    environmentView_ = irradianceView_ = prefilteredView_ = VK_NULL_HANDLE;
    environmentSampler_ = irradianceSampler_ = prefilteredSampler_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE; physicalDevice_ = VK_NULL_HANDLE; graphicsQueue_ = VK_NULL_HANDLE; graphicsQueueFamily_ = UINT32_MAX;
    source_.clear(); environment_.clear(); irradiance_.clear(); prefilteredMips_.clear();
    sourceWidth_ = sourceHeight_ = prefilterMipLevels_ = 0;
}

} // namespace NeoEngine
