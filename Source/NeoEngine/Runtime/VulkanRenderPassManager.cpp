#include "Runtime/VulkanRenderPassManager.h"

#include <array>
#include <utility>

namespace NeoEngine {
namespace {

bool CreateRenderPass(VkDevice device, const RenderPassConfig& config, bool useDepth, VkRenderPass* outRenderPass) {
    if (device == VK_NULL_HANDLE || outRenderPass == nullptr ||
        config.colorFormat == VK_FORMAT_UNDEFINED || (useDepth && config.depthFormat == VK_FORMAT_UNDEFINED)) {
        return false;
    }

    VkAttachmentDescription attachments[2]{};
    attachments[0].format = config.colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = config.colorLoadOp;
    attachments[0].storeOp = config.colorStoreOp;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = config.initialLayout;
    attachments[0].finalLayout = config.finalLayout;

    VkAttachmentReference colorReference{};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference{};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;
    if (useDepth) {
        subpass.pDepthStencilAttachment = &depthReference;
    }

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    if (useDepth) {
        dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                   VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    uint32_t attachmentCount = 1;
    if (useDepth) {
        attachments[1].format = config.depthFormat;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = config.depthLoadOp;
        attachments[1].storeOp = config.depthStoreOp;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachmentCount = 2;
    }

    VkRenderPassCreateInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    info.attachmentCount = attachmentCount;
    info.pAttachments = attachments;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;

    return vkCreateRenderPass(device, &info, nullptr, outRenderPass) == VK_SUCCESS;
}

} // namespace

VulkanRenderPassManager::~VulkanRenderPassManager() {
    Destroy();
}

VulkanRenderPassManager::VulkanRenderPassManager(VulkanRenderPassManager&& other) noexcept
    : device_(other.device_),
      renderPass_(other.renderPass_),
      framebuffer_(other.framebuffer_),
      width_(other.width_),
      height_(other.height_),
      config_(other.config_) {
    other.device_ = VK_NULL_HANDLE;
    other.renderPass_ = VK_NULL_HANDLE;
    other.framebuffer_ = VK_NULL_HANDLE;
    other.width_ = 0;
    other.height_ = 0;
    other.config_ = RenderPassConfig{};
}

VulkanRenderPassManager& VulkanRenderPassManager::operator=(VulkanRenderPassManager&& other) noexcept {
    if (this != &other) {
        Destroy();
        device_ = other.device_;
        renderPass_ = other.renderPass_;
        framebuffer_ = other.framebuffer_;
        width_ = other.width_;
        height_ = other.height_;
        config_ = other.config_;

        other.device_ = VK_NULL_HANDLE;
        other.renderPass_ = VK_NULL_HANDLE;
        other.framebuffer_ = VK_NULL_HANDLE;
        other.width_ = 0;
        other.height_ = 0;
        other.config_ = RenderPassConfig{};
    }
    return *this;
}

bool VulkanRenderPassManager::Initialize(VkDevice device, const RenderPassConfig& config) {
    if (device == VK_NULL_HANDLE || config.colorFormat == VK_FORMAT_UNDEFINED ||
        (config.enableDepth && config.depthFormat == VK_FORMAT_UNDEFINED)) {
        return false;
    }

    Destroy();
    device_ = device;
    config_ = config;

    if (!CreateRenderPass(device_, config_, config_.enableDepth, &renderPass_)) {
        Destroy();
        return false;
    }
    return true;
}

bool VulkanRenderPassManager::CreateFramebuffer(
    VkImageView colorImageView, VkImageView depthImageView, uint32_t width, uint32_t height) {
    if (device_ == VK_NULL_HANDLE || renderPass_ == VK_NULL_HANDLE ||
        colorImageView == VK_NULL_HANDLE || width == 0 || height == 0) {
        return false;
    }

    if (framebuffer_ != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device_, framebuffer_, nullptr);
        framebuffer_ = VK_NULL_HANDLE;
    }

    const bool useDepth = config_.enableDepth && depthImageView != VK_NULL_HANDLE;

    // A framebuffer's attachment count must exactly match its render-pass
    // attachment contract. Rebuild the pass only when the caller supplies a
    // different depth-view contract than the initialized configuration.
    if (useDepth != config_.enableDepth) {
        vkDestroyRenderPass(device_, renderPass_, nullptr);
        renderPass_ = VK_NULL_HANDLE;
        if (!CreateRenderPass(device_, config_, useDepth, &renderPass_)) {
            return false;
        }
    }

    const std::array<VkImageView, 2> attachments{colorImageView, depthImageView};
    VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    info.renderPass = renderPass_;
    info.attachmentCount = useDepth ? 2U : 1U;
    info.pAttachments = attachments.data();
    info.width = width;
    info.height = height;
    info.layers = 1;

    if (vkCreateFramebuffer(device_, &info, nullptr, &framebuffer_) != VK_SUCCESS) {
        framebuffer_ = VK_NULL_HANDLE;
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
        }
        if (renderPass_ != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device_, renderPass_, nullptr);
        }
    }
    framebuffer_ = VK_NULL_HANDLE;
    renderPass_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    width_ = 0;
    height_ = 0;
    config_ = RenderPassConfig{};
}

} // namespace NeoEngine
