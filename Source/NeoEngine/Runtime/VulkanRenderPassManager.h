#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace NeoEngine {

struct RenderPassConfig {
    VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    VkAttachmentLoadOp colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp colorStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkAttachmentLoadOp depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    bool enableDepth = true;
};

class VulkanRenderPassManager {
public:
    VulkanRenderPassManager() = default;
    ~VulkanRenderPassManager();

    VulkanRenderPassManager(const VulkanRenderPassManager&) = delete;
    VulkanRenderPassManager& operator=(const VulkanRenderPassManager&) = delete;

    VulkanRenderPassManager(VulkanRenderPassManager&& other) noexcept;
    VulkanRenderPassManager& operator=(VulkanRenderPassManager&& other) noexcept;

    bool Initialize(VkDevice device, const RenderPassConfig& config);
    bool CreateFramebuffer(VkImageView colorImageView, VkImageView depthImageView, uint32_t width, uint32_t height);

    void Destroy();

    [[nodiscard]] VkRenderPass GetRenderPass() const { return renderPass_; }
    [[nodiscard]] VkFramebuffer GetFramebuffer() const { return framebuffer_; }
    [[nodiscard]] uint32_t GetWidth() const { return width_; }
    [[nodiscard]] uint32_t GetHeight() const { return height_; }
    [[nodiscard]] bool IsValid() const { return renderPass_ != VK_NULL_HANDLE; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

} // namespace NeoEngine
