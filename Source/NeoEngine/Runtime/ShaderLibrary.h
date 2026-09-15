#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace NeoEngine {

enum class TextureSampler : uint8_t {
    PointClamp = 0,
    LinearClamp,
    LinearRepeat,
    LinearClampMipmap,
    Count
};

struct ShaderVariantKey {
    std::string name;
    VkShaderStageFlagBits stage = VK_SHADER_STAGE_VERTEX_BIT;
    std::string variant;

    bool operator==(const ShaderVariantKey& other) const noexcept {
        return stage == other.stage && name == other.name && variant == other.variant;
    }
};

struct ShaderVariantKeyHash {
    size_t operator()(const ShaderVariantKey& key) const noexcept;
};

class ShaderLibrary {
public:
    ShaderLibrary() = default;
    ~ShaderLibrary();

    ShaderLibrary(const ShaderLibrary&) = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;
    ShaderLibrary(ShaderLibrary&& other) noexcept;
    ShaderLibrary& operator=(ShaderLibrary&& other) noexcept;

    bool Initialize(VkDevice device);

    // Runtime consumes validated SPIR-V. GLSL/HLSL compilation remains an
    // offline build concern; this boundary centralizes module creation,
    // variant identity, caching, and lifetime management.
    VkShaderModule CompileShader(std::string_view name,
                                 VkShaderStageFlagBits stage,
                                 const std::vector<uint32_t>& spirv,
                                 std::string_view variant = {});

    VkShaderModule GetShaderModule(std::string_view name,
                                   VkShaderStageFlagBits stage,
                                   std::string_view variant = {}) const;

    VkSampler GetSampler(TextureSampler type) const;

    bool CreateSampler(TextureSampler type,
                       VkFilter filter,
                       VkSamplerAddressMode addressMode,
                       bool mipmapped = false);

    bool ReloadShader(std::string_view name,
                      VkShaderStageFlagBits stage,
                      const std::vector<uint32_t>& spirv,
                      std::string_view variant = {});

    void ClearShaders();
    void Destroy();

    [[nodiscard]] VkDevice GetDevice() const noexcept { return device_; }
    [[nodiscard]] bool IsValid() const noexcept { return device_ != VK_NULL_HANDLE; }
    [[nodiscard]] size_t ShaderCount() const noexcept { return shaders_.size(); }

private:
    bool CreateDefaultSamplers();
    void DestroySamplers();
    static bool IsValidSpirV(const std::vector<uint32_t>& spirv);

    VkDevice device_ = VK_NULL_HANDLE;
    std::unordered_map<ShaderVariantKey, VkShaderModule, ShaderVariantKeyHash> shaders_;
    std::vector<VkSampler> samplers_;
};

} // namespace NeoEngine
