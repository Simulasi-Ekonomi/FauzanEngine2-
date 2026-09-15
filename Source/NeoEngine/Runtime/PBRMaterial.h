#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <memory>

namespace NeoEngine {

enum class TextureSlot : uint8_t {
    BaseColor = 0,
    Normal = 1,
    Metallic = 2,
    Roughness = 3,
    AmbientOcclusion = 4,
    Count = 5
};

struct PBRMaterialParams {
    glm::vec3 baseColor = {0.8f, 0.8f, 0.8f};
    float metallic = 0.0f;              // [0, 1]
    float roughness = 0.5f;             // [0, 1]
    float normalMapStrength = 1.0f;
    float aoStrength = 1.0f;
    glm::uint32_t flags = 0;
    
    // Padding to 256 bytes for GPU alignment
    float _padding[50];
};

class PBRMaterial {
public:
    PBRMaterial() noexcept = default;
    ~PBRMaterial() noexcept;
    
    PBRMaterial(const PBRMaterial&) = delete;
    PBRMaterial& operator=(const PBRMaterial&) = delete;
    PBRMaterial(PBRMaterial&&) noexcept = default;
    PBRMaterial& operator=(PBRMaterial&&) noexcept = default;
    
    // Load material from JSON file
    [[nodiscard]] bool Load(const std::string& filepath) noexcept;
    
    // Set texture for a slot
    [[nodiscard]] bool SetTexture(TextureSlot slot, VkImageView imageView) noexcept;
    
    // Get current material parameters
    [[nodiscard]] const PBRMaterialParams& GetParams() const noexcept;
    
    // Update material parameters
    void SetParams(const PBRMaterialParams& params) noexcept;
    
    // Get GPU descriptor set for this material
    [[nodiscard]] VkDescriptorSet GetDescriptorSet() const noexcept;
    
    // Get material name
    [[nodiscard]] const std::string& GetName() const noexcept { return name_; }
    
    // Check if material is fully loaded
    [[nodiscard]] bool IsValid() const noexcept;

private:
    std::string name_;
    PBRMaterialParams params_;
    std::array<VkImageView, static_cast<size_t>(TextureSlot::Count)> textures_;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
    bool isValid_ = false;
};

} // namespace NeoEngine
