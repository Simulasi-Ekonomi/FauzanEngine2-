#include "Runtime/VulkanRenderPassManager.h"
#include <utility>

namespace NeoEngine {

VulkanRenderPassManager::~VulkanRenderPassManager() {
    Destroy();
}

VulkanRenderPassManager::VulkanRenderPassManager(VulkanRenderPassManager&& other) noexcept {
    device_ = other.device_;
    renderPass_ = other.renderPass_;
    framebuffer_ = other.framebuffer_;
    width_ = other.width_;
    height_ = other.height_;

    other.device_ = VK_NULL_HANDLE;
    other.renderPass_ = VK_NULL_HANDLE;
    other.framebuffer_ = VK_NULL_HANDLE;
    other.width_ = 0;
    other.height_ = 0;
}

VulkanRenderPassManager& VulkanRenderPassManager::operator=(VulkanRenderPassManager&& other) noexcept {
    if (this != &other) {
        Destroy();

        device_ = other.device_;
        renderPass_ = other.renderPass_;
        framebuffer_ = other.framebuffer_;
        width_ = other.width_;
        height_ = other.height_;

        other.device_ = VK_NULL_HANDLE;
        other.renderPass_ = VK_NULL_HANDLE;
        other.framebuffer_ = VK_NULL_HANDLE;
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

bool VulkanRenderPassManager::Initialize(VkDevice device, const RenderPassConfig& config) {
    if (device == VK_NULL_HANDLE) {
        return false;
    }

    Destroy();
    device_ = device;

    std::vector<VkAttachmentDescription> attachments;

    // 1. Color Attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = config.colorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = config.colorLoadOp;
    colorAttachment.storeOp = config.colorStoreOp;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = config.initialLayout;
    colorAttachment.finalLayout = config.finalLayout;
    attachments.push_back(colorAttachment);

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // 2. Depth Attachment
    VkAttachmentReference depthAttachmentRef{};
    if (config.enableDepth) {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = config.depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = config.depthLoadOp;
        depthAttachment.storeOp = config.depthStoreOp;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(depthAttachment);

        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    // Subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    if (config.enableDepth) {
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
    }

    // Subpass Dependency
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | (config.enableDepth ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT : 0);
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | (config.enableDepth ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT : 0);
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | (config.enableDepth ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);

    // Create Render Pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    return true;
}

bool VulkanRenderPassManager::CreateFramebuffer(VkImageView colorImageView, VkImageView depthImageView, uint32_t width, uint32_t height) {
    if (device_ == VK_NULL_HANDLE || renderPass_ == VK_NULL_HANDLE || colorImageView == VK_NULL_HANDLE || width == 0 || height == 0) {
        return false;
    }

    if (framebuffer_ != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device_, framebuffer_, nullptr);
        framebuffer_ = VK_NULL_HANDLE;
    }

    std::vector<VkImageView> attachments = { colorImageView };
    if (depthImageView != VK_NULL_HANDLE) {
        attachments.push_back(depthImageView);
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass_;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &framebuffer_) != VK_SUCCESS) {
        return false;
    }

    width_ = width;
    height_ = height;
    return true;
}

void VulkanRenderPassManager::Destroy() {
    if (device_ != VK_NULL_HANDLE) {
        if (framebuffer_ != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device_, framebuffer_, nullptr);
            framebuffer_ = VK_NULL_HANDLE;
        }
        if (renderPass_ != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device_, renderPass_, nullptr);
            renderPass_ = VK_NULL_HANDLE;
        }
        device_ = VK_NULL_HANDLE;
    }
    width_ = 0;
    height_ = 0;
}

} // namespace NeoEngine
