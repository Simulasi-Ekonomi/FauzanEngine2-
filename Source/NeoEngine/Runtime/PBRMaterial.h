#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace NeoEngine {

class VulkanDescriptorManager;

enum class TextureSlot : uint8_t {
    BaseColor = 0,
    Normal = 1,
    Metallic = 2,
    Roughness = 3,
    AmbientOcclusion = 4,
    Count = 5
};

// GPU-facing material payload. Every member follows 16-byte UBO alignment.
struct alignas(16) PBRMaterialParams {
    glm::vec4 baseColor = {0.8f, 0.8f, 0.8f, 1.0f};
    glm::vec4 materialFactors = {0.0f, 0.5f, 1.0f, 1.0f}; // metallic, roughness, normal strength, AO strength
    uint32_t flags = 0;
    uint32_t _padding[3] = {};
};

static_assert(sizeof(PBRMaterialParams) == 48, "PBRMaterialParams must remain a 48-byte Vulkan UBO payload");
static_assert(offsetof(PBRMaterialParams, baseColor) == 0);
static_assert(offsetof(PBRMaterialParams, materialFactors) == 16);
static_assert(offsetof(PBRMaterialParams, flags) == 32);

class PBRMaterial {
public:
    PBRMaterial() noexcept;
    ~PBRMaterial() noexcept;

    PBRMaterial(const PBRMaterial&) = delete;
    PBRMaterial& operator=(const PBRMaterial&) = delete;
    PBRMaterial(PBRMaterial&&) noexcept = default;
    PBRMaterial& operator=(PBRMaterial&&) noexcept = default;

    [[nodiscard]] bool Load(const std::string& filepath) noexcept;
    [[nodiscard]] bool SetTexture(TextureSlot slot, VkImageView imageView) noexcept;
    [[nodiscard]] bool SetTexture(TextureSlot slot, VkImageView imageView, VkSampler sampler) noexcept;
    [[nodiscard]] bool SetTextureReference(TextureSlot slot, const std::string& assetPath) noexcept;

    [[nodiscard]] const PBRMaterialParams& GetParams() const noexcept { return params_; }
    void SetParams(const PBRMaterialParams& params) noexcept { params_ = params; }

    [[nodiscard]] VkImageView GetTextureView(TextureSlot slot) const noexcept;
    [[nodiscard]] VkSampler GetTextureSampler(TextureSlot slot) const noexcept;
    [[nodiscard]] const std::string& GetTextureReference(TextureSlot slot) const noexcept;

    [[nodiscard]] bool InitializeDescriptor(VulkanDescriptorManager& manager) noexcept;
    [[nodiscard]] bool SetParameterBuffer(VkBuffer buffer, VkDeviceSize range = sizeof(PBRMaterialParams)) noexcept;
    [[nodiscard]] bool UpdateDescriptor() noexcept;

    [[nodiscard]] VkDescriptorSet GetDescriptorSet() const noexcept { return descriptorSet_; }
    [[nodiscard]] const std::string& GetName() const noexcept { return name_; }
    [[nodiscard]] bool IsValid() const noexcept { return isValid_; }
    [[nodiscard]] bool HasCompleteTextureBindings() const noexcept;

private:
    static constexpr size_t kTextureCount = static_cast<size_t>(TextureSlot::Count);

    std::string name_;
    PBRMaterialParams params_{};
    std::array<VkImageView, kTextureCount> textures_{};
    std::array<VkSampler, kTextureCount> samplers_{};
    std::array<std::string, kTextureCount> textureReferences_{};
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
    VkBuffer parameterBuffer_ = VK_NULL_HANDLE;
    VkDeviceSize parameterBufferRange_ = sizeof(PBRMaterialParams);
    VulkanDescriptorManager* descriptorManager_ = nullptr;
    bool isValid_ = false;
};

} // namespace NeoEngine
