#include "PBRMaterial.h"

#include "Runtime/VulkanDescriptorManager.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>

#include <json/json.h>

namespace NeoEngine {

namespace {
bool ReadFloat(const Json::Value& value, float& target) {
    if (!value.isNumeric()) return false;
    target = value.asFloat();
    return std::isfinite(target);
}

bool ReadColor(const Json::Value& value, glm::vec4& target) {
    if (!value.isArray() || (value.size() != 3 && value.size() != 4)) return false;
    for (Json::ArrayIndex i = 0; i < value.size(); ++i) {
        if (!value[i].isNumeric()) return false;
        target[static_cast<int>(i)] = value[i].asFloat();
    }
    if (value.size() == 3) target.w = 1.0f;
    return true;
}

size_t SlotIndex(TextureSlot slot) {
    return static_cast<size_t>(slot);
}
} // namespace

PBRMaterial::PBRMaterial() noexcept {
    textures_.fill(VK_NULL_HANDLE);
    samplers_.fill(VK_NULL_HANDLE);
}

PBRMaterial::~PBRMaterial() noexcept = default;

bool PBRMaterial::Load(const std::string& filepath) noexcept {
    if (filepath.empty()) return false;

    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    Json::Value materialJson;
    std::string errors;
    if (!Json::parseFromStream(builder, file, &materialJson, &errors) || !materialJson.isObject()) {
        return false;
    }

    const Json::Value& name = materialJson["name"];
    if (!name.isString() || name.asString().empty()) return false;

    name_ = name.asString();
    params_ = PBRMaterialParams{};

    const Json::Value& params = materialJson["parameters"];
    if (params.isObject()) {
        if (params.isMember("baseColor")) ReadColor(params["baseColor"], params_.baseColor);

        float value = 0.0f;
        if (params.isMember("metallic") && ReadFloat(params["metallic"], value)) {
            params_.materialFactors.x = std::clamp(value, 0.0f, 1.0f);
        }
        if (params.isMember("roughness") && ReadFloat(params["roughness"], value)) {
            params_.materialFactors.y = std::clamp(value, 0.045f, 1.0f);
        }
        if (params.isMember("normalMapStrength") && ReadFloat(params["normalMapStrength"], value)) {
            params_.materialFactors.z = std::max(value, 0.0f);
        }
        if (params.isMember("aoStrength") && ReadFloat(params["aoStrength"], value)) {
            params_.materialFactors.w = std::clamp(value, 0.0f, 1.0f);
        }
        if (params.isMember("flags") && params["flags"].isUInt()) {
            params_.flags = params["flags"].asUInt();
        }
    }

    const Json::Value& textures = materialJson["textures"];
    if (textures.isObject()) {
        constexpr std::array<const char*, 5> keys = {
            "baseColor", "normal", "metallic", "roughness", "ambientOcclusion"
        };
        for (size_t i = 0; i < keys.size(); ++i) {
            if (textures[keys[i]].isString()) textureReferences_[i] = textures[keys[i]].asString();
        }
    }

    isValid_ = true;
    return true;
}

bool PBRMaterial::SetTexture(TextureSlot slot, VkImageView imageView) noexcept {
    return SetTexture(slot, imageView, VK_NULL_HANDLE);
}

bool PBRMaterial::SetTexture(TextureSlot slot, VkImageView imageView, VkSampler sampler) noexcept {
    const size_t index = SlotIndex(slot);
    if (index >= kTextureCount || imageView == VK_NULL_HANDLE) return false;
    textures_[index] = imageView;
    samplers_[index] = sampler;
    return true;
}

bool PBRMaterial::SetTextureReference(TextureSlot slot, const std::string& assetPath) noexcept {
    const size_t index = SlotIndex(slot);
    if (index >= kTextureCount || assetPath.empty()) return false;
    textureReferences_[index] = assetPath;
    return true;
}

VkImageView PBRMaterial::GetTextureView(TextureSlot slot) const noexcept {
    const size_t index = SlotIndex(slot);
    return index < kTextureCount ? textures_[index] : VK_NULL_HANDLE;
}

VkSampler PBRMaterial::GetTextureSampler(TextureSlot slot) const noexcept {
    const size_t index = SlotIndex(slot);
    return index < kTextureCount ? samplers_[index] : VK_NULL_HANDLE;
}

const std::string& PBRMaterial::GetTextureReference(TextureSlot slot) const noexcept {
    static const std::string empty;
    const size_t index = SlotIndex(slot);
    return index < kTextureCount ? textureReferences_[index] : empty;
}

bool PBRMaterial::InitializeDescriptor(VulkanDescriptorManager& manager) noexcept {
    if (!manager.IsValid()) return false;
    descriptorManager_ = &manager;
    descriptorSet_ = manager.AllocateSet();
    return descriptorSet_ != VK_NULL_HANDLE;
}

bool PBRMaterial::SetParameterBuffer(VkBuffer buffer, VkDeviceSize range) noexcept {
    if (buffer == VK_NULL_HANDLE || range == 0) return false;
    parameterBuffer_ = buffer;
    parameterBufferRange_ = range;
    return true;
}

bool PBRMaterial::UpdateDescriptor() noexcept {
    if (descriptorManager_ == nullptr || descriptorSet_ == VK_NULL_HANDLE || parameterBuffer_ == VK_NULL_HANDLE) return false;

    descriptorManager_->UpdateBufferBinding(
        descriptorSet_, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        parameterBuffer_, 0, parameterBufferRange_);

    for (size_t i = 0; i < kTextureCount; ++i) {
        if (textures_[i] == VK_NULL_HANDLE || samplers_[i] == VK_NULL_HANDLE) return false;
        descriptorManager_->UpdateImageBinding(
            descriptorSet_, static_cast<uint32_t>(i + 1),
            textures_[i], samplers_[i]);
    }
    return true;
}

bool PBRMaterial::HasCompleteTextureBindings() const noexcept {
    for (size_t i = 0; i < kTextureCount; ++i) {
        if (textures_[i] == VK_NULL_HANDLE || samplers_[i] == VK_NULL_HANDLE) return false;
    }
    return true;
}

} // namespace NeoEngine
