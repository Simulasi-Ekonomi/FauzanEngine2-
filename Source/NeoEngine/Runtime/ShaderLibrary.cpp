#include "Runtime/ShaderLibrary.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>

namespace NeoEngine {

std::size_t ShaderVariantKeyHash::operator()(const ShaderVariantKey& key) const noexcept {
    const std::size_t hName = std::hash<std::string>{}(key.name);
    const std::size_t hStage = std::hash<uint32_t>{}(static_cast<uint32_t>(key.stage));
    const std::size_t hVariant = std::hash<std::string>{}(key.variant);
    return hName ^ (hStage + static_cast<std::size_t>(0x9e3779b9U) + (hName << 6U) + (hName >> 2U)) ^
           (hVariant + static_cast<std::size_t>(0x9e3779b9U) + (hStage << 6U) + (hStage >> 2U));
}

ShaderLibrary::~ShaderLibrary() {
    Destroy();
}

ShaderLibrary::ShaderLibrary(ShaderLibrary&& other) noexcept
    : device_(other.device_), shaders_(std::move(other.shaders_)), samplers_(std::move(other.samplers_)) {
    other.device_ = VK_NULL_HANDLE;
    other.shaders_.clear();
    other.samplers_.clear();
}

ShaderLibrary& ShaderLibrary::operator=(ShaderLibrary&& other) noexcept {
    if (this != &other) {
        Destroy();
        device_ = other.device_;
        shaders_ = std::move(other.shaders_);
        samplers_ = std::move(other.samplers_);
        other.device_ = VK_NULL_HANDLE;
        other.shaders_.clear();
        other.samplers_.clear();
    }
    return *this;
}

bool ShaderLibrary::Initialize(VkDevice device) {
    if (device == VK_NULL_HANDLE) return false;
    if (device_ == device) return true;
    Destroy();
    device_ = device;
    if (!CreateDefaultSamplers()) {
        Destroy();
        return false;
    }
    return true;
}

bool ShaderLibrary::IsValidSpirV(const std::vector<uint32_t>& spirv) {
    // Validate the SPIR-V container/header and instruction boundaries before
    // passing the module to Vulkan. Semantic validation remains the job of
    // spirv-val in CI and vkCreateShaderModule at runtime.
    constexpr uint32_t kMagic = 0x07230203U;
    constexpr uint32_t kMaxSupportedVersion = 0x00010600U; // SPIR-V 1.6

    if (spirv.size() < 5 || spirv[0] != kMagic || spirv[1] > kMaxSupportedVersion ||
        spirv[3] == 0U || spirv[4] != 0U) {
        return false;
    }

    std::size_t cursor = 5;
    while (cursor < spirv.size()) {
        const uint32_t instruction = spirv[cursor];
        const uint32_t wordCount = instruction >> 16U;
        if (wordCount == 0U || static_cast<std::size_t>(wordCount) > spirv.size() - cursor) {
            return false;
        }
        cursor += wordCount;
    }

    return cursor == spirv.size();
}

VkShaderModule ShaderLibrary::CompileShader(std::string_view name,
                                            VkShaderStageFlagBits stage,
                                            const std::vector<uint32_t>& spirv,
                                            std::string_view variant) {
    if (device_ == VK_NULL_HANDLE || name.empty() || !IsValidSpirV(spirv)) {
        return VK_NULL_HANDLE;
    }

    ShaderVariantKey key{std::string(name), stage, std::string(variant)};
    const auto found = shaders_.find(key);
    if (found != shaders_.end()) return found->second;

    VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();

    VkShaderModule module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device_, &createInfo, nullptr, &module) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    shaders_.emplace(std::move(key), module);
    return module;
}

VkShaderModule ShaderLibrary::GetShaderModule(std::string_view name,
                                              VkShaderStageFlagBits stage,
                                              std::string_view variant) const {
    ShaderVariantKey key{std::string(name), stage, std::string(variant)};
    const auto found = shaders_.find(key);
    return found == shaders_.end() ? VK_NULL_HANDLE : found->second;
}

VkSampler ShaderLibrary::GetSampler(TextureSampler type) const {
    const auto index = static_cast<std::size_t>(type);
    return index < samplers_.size() ? samplers_[index] : VK_NULL_HANDLE;
}

bool ShaderLibrary::CreateSampler(TextureSampler type,
                                  VkFilter filter,
                                  VkSamplerAddressMode addressMode,
                                  bool mipmapped) {
    if (device_ == VK_NULL_HANDLE || type == TextureSampler::Count) return false;

    const std::size_t index = static_cast<std::size_t>(type);
    if (samplers_.size() < static_cast<std::size_t>(TextureSampler::Count)) {
        samplers_.resize(static_cast<std::size_t>(TextureSampler::Count), VK_NULL_HANDLE);
    }
    if (samplers_[index] != VK_NULL_HANDLE) return true;

    VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    info.magFilter = filter;
    info.minFilter = filter;
    info.mipmapMode = mipmapped ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.addressModeU = addressMode;
    info.addressModeV = addressMode;
    info.addressModeW = addressMode;
    info.mipLodBias = 0.0f;
    info.anisotropyEnable = VK_FALSE;
    info.maxAnisotropy = 1.0f;
    info.compareEnable = VK_FALSE;
    info.minLod = 0.0f;
    info.maxLod = mipmapped ? 1000.0f : 0.0f;
    info.unnormalizedCoordinates = VK_FALSE;

    return vkCreateSampler(device_, &info, nullptr, &samplers_[index]) == VK_SUCCESS;
}

bool ShaderLibrary::CreateDefaultSamplers() {
    if (samplers_.size() != static_cast<std::size_t>(TextureSampler::Count)) {
        samplers_.assign(static_cast<std::size_t>(TextureSampler::Count), VK_NULL_HANDLE);
    }

    return CreateSampler(TextureSampler::PointClamp, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) &&
           CreateSampler(TextureSampler::LinearClamp, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) &&
           CreateSampler(TextureSampler::LinearRepeat, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT) &&
           CreateSampler(TextureSampler::LinearClampMipmap, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, true);
}

bool ShaderLibrary::ReloadShader(std::string_view name,
                                 VkShaderStageFlagBits stage,
                                 const std::vector<uint32_t>& spirv,
                                 std::string_view variant) {
    if (device_ == VK_NULL_HANDLE || name.empty() || !IsValidSpirV(spirv)) return false;

    ShaderVariantKey key{std::string(name), stage, std::string(variant)};
    const auto found = shaders_.find(key);
    if (found != shaders_.end()) {
        vkDestroyShaderModule(device_, found->second, nullptr);
        shaders_.erase(found);
    }
    return CompileShader(name, stage, spirv, variant) != VK_NULL_HANDLE;
}

void ShaderLibrary::ClearShaders() {
    if (device_ == VK_NULL_HANDLE) {
        shaders_.clear();
        return;
    }
    for (const auto& entry : shaders_) {
        if (entry.second != VK_NULL_HANDLE) vkDestroyShaderModule(device_, entry.second, nullptr);
    }
    shaders_.clear();
}

void ShaderLibrary::DestroySamplers() {
    if (device_ != VK_NULL_HANDLE) {
        for (VkSampler sampler : samplers_) {
            if (sampler != VK_NULL_HANDLE) vkDestroySampler(device_, sampler, nullptr);
        }
    }
    samplers_.clear();
}

void ShaderLibrary::Destroy() {
    ClearShaders();
    DestroySamplers();
    device_ = VK_NULL_HANDLE;
}

} // namespace NeoEngine
