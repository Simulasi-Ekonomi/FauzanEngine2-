#include "BRDFLut.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include <glm/gtc/packing.hpp>

namespace NeoEngine {

namespace {
constexpr uint32_t kSampleCount = 256;
constexpr float kPi = 3.14159265358979323846f;

float RadicalInverseVdC(uint32_t bits) noexcept {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return static_cast<float>(bits) * 2.3283064365386963e-10f;
}

glm::vec2 Hammersley(uint32_t i, uint32_t count) noexcept {
    return {static_cast<float>(i) / static_cast<float>(count), RadicalInverseVdC(i)};
}

glm::vec3 ImportanceSampleGGX(glm::vec2 xi, float roughness) noexcept {
    const float alpha = roughness * roughness;
    const float alpha2 = alpha * alpha;
    const float phi = 2.0f * kPi * xi.x;
    const float cosTheta = std::sqrt((1.0f - xi.y) / (1.0f + (alpha2 - 1.0f) * xi.y));
    const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));

    return glm::normalize(glm::vec3(
        sinTheta * std::cos(phi),
        sinTheta * std::sin(phi),
        cosTheta));
}

float GeometrySchlickGGX(float ndotv, float roughness) noexcept {
    const float alpha = roughness * roughness;
    const float k = alpha * 0.5f;
    return ndotv / (ndotv * (1.0f - k) + k);
}

float GeometrySmith(float ndotv, float ndotl, float roughness) noexcept {
    return GeometrySchlickGGX(ndotv, roughness) * GeometrySchlickGGX(ndotl, roughness);
}

} // namespace

bool BRDFLut::Initialize(VkDevice device,
                         VkPhysicalDevice physicalDevice,
                         VkQueue graphicsQueue,
                         uint32_t graphicsQueueFamily) noexcept {
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE ||
        graphicsQueue == VK_NULL_HANDLE || graphicsQueueFamily == UINT32_MAX) {
        return false;
    }

    Destroy();
    device_ = device;
    physicalDevice_ = physicalDevice;
    graphicsQueue_ = graphicsQueue;
    graphicsQueueFamily_ = graphicsQueueFamily;
    return true;
}

bool BRDFLut::Generate() noexcept {
    if (device_ == VK_NULL_HANDLE || physicalDevice_ == VK_NULL_HANDLE ||
        graphicsQueue_ == VK_NULL_HANDLE || graphicsQueueFamily_ == UINT32_MAX) {
        return false;
    }

    isValid_ = false;
    texture_.Destroy();

    if (!texture_.Initialize(device_, physicalDevice_, LUT_RESOLUTION, LUT_RESOLUTION, LUT_FORMAT)) {
        return false;
    }

    // R16G16_SFLOAT is exactly 32 bits per texel; pack the two LUT channels as half floats.
    std::vector<uint32_t> lutData(static_cast<size_t>(LUT_RESOLUTION) * LUT_RESOLUTION);
    for (uint32_t y = 0; y < LUT_RESOLUTION; ++y) {
        const float roughness = std::max(
            static_cast<float>(y) / static_cast<float>(LUT_RESOLUTION - 1), 0.001f);
        for (uint32_t x = 0; x < LUT_RESOLUTION; ++x) {
            const float ndotv = std::max(
                static_cast<float>(x) / static_cast<float>(LUT_RESOLUTION - 1), 0.001f);
            const glm::vec2 value = IntegrateBRDF(roughness, ndotv);
            lutData[static_cast<size_t>(y) * LUT_RESOLUTION + x] =
                glm::packHalf2x16(value);
        }
    }

    if (!texture_.UploadPixels(
            graphicsQueue_, graphicsQueueFamily_, physicalDevice_,
            lutData.data(), static_cast<VkDeviceSize>(lutData.size() * sizeof(uint32_t)))) {
        texture_.Destroy();
        return false;
    }

    if (!texture_.CreateSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)) {
        texture_.Destroy();
        return false;
    }

    isValid_ = true;
    return true;
}

void BRDFLut::Destroy() noexcept {
    texture_.Destroy();
    device_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
    graphicsQueue_ = VK_NULL_HANDLE;
    graphicsQueueFamily_ = UINT32_MAX;
    isValid_ = false;
}

glm::vec2 BRDFLut::IntegrateBRDF(float roughness, float ndotv) noexcept {
    roughness = std::clamp(roughness, 0.001f, 1.0f);
    ndotv = std::clamp(ndotv, 0.001f, 1.0f);

    const glm::vec3 v(
        std::sqrt(std::max(0.0f, 1.0f - ndotv * ndotv)),
        0.0f,
        ndotv);

    float a = 0.0f;
    float b = 0.0f;

    for (uint32_t i = 0; i < kSampleCount; ++i) {
        const glm::vec2 xi = Hammersley(i, kSampleCount);
        const glm::vec3 h = ImportanceSampleGGX(xi, roughness);
        const glm::vec3 l = glm::normalize(2.0f * glm::dot(v, h) * h - v);

        const float ndotl = std::max(l.z, 0.0f);
        const float ndoth = std::max(h.z, 0.0f);
        const float vdoth = std::max(glm::dot(v, h), 0.0f);

        if (ndotl <= 0.0f || ndoth <= 0.0f || vdoth <= 0.0f) continue;

        const float geometry = GeometrySmith(ndotv, ndotl, roughness);
        const float geometryVisibility = (geometry * vdoth) /
                                          std::max(ndoth * ndotv, 1.0e-5f);
        const float fresnel = std::pow(1.0f - vdoth, 5.0f);

        a += (1.0f - fresnel) * geometryVisibility;
        b += fresnel * geometryVisibility;
    }

    const float invSampleCount = 1.0f / static_cast<float>(kSampleCount);
    return {a * invSampleCount, b * invSampleCount};
}

} // namespace NeoEngine
