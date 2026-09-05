#include "PBRMaterial.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace NeoEngine {

using json = nlohmann::json;

PBRMaterial::~PBRMaterial() noexcept {
    // Descriptor set is managed by VulkanDescriptorManager
    // Texture views are managed by VulkanGPUTexture
    // No cleanup needed here (RAII handled elsewhere)
}

bool PBRMaterial::Load(const std::string& filepath) noexcept {
    if (filepath.empty()) {
        return false;
    }

    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        json materialJson;
        file >> materialJson;
        file.close();

        // Load name
        if (materialJson.contains("name")) {
            name_ = materialJson["name"].get<std::string>();
        }

        // Load parameters
        if (materialJson.contains("parameters")) {
            const auto& params = materialJson["parameters"];
            if (params.contains("normalMapStrength")) {
                params_.normalMapStrength = params["normalMapStrength"].get<float>();
            }
            if (params.contains("aoStrength")) {
                params_.aoStrength = params["aoStrength"].get<float>();
            }
        }

        // Material is valid if name is set
        isValid_ = !name_.empty();
        return isValid_;

    } catch (const std::exception& e) {
        return false;
    }
}

bool PBRMaterial::SetTexture(TextureSlot slot, VkImageView imageView) noexcept {
    if (imageView == VK_NULL_HANDLE) {
        return false;
    }

    size_t slotIndex = static_cast<size_t>(slot);
    if (slotIndex >= textures_.size()) {
        return false;
    }

    textures_[slotIndex] = imageView;
    return true;
}

const PBRMaterialParams& PBRMaterial::GetParams() const noexcept {
    return params_;
}

void PBRMaterial::SetParams(const PBRMaterialParams& params) noexcept {
    params_ = params;
}

VkDescriptorSet PBRMaterial::GetDescriptorSet() const noexcept {
    return descriptorSet_;
}

bool PBRMaterial::IsValid() const noexcept {
    return isValid_;
}

} // namespace NeoEngine
