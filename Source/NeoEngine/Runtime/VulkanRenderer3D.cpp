#include "Runtime/VulkanRenderer3D.h"
#include <cstring>
#include <utility>

namespace NeoEngine {

VulkanRenderer3D::~VulkanRenderer3D() {
    Destroy();
}

VulkanRenderer3D::VulkanRenderer3D(VulkanRenderer3D&& other) noexcept {
    context_ = other.context_;
    renderPassManager_ = std::move(other.renderPassManager_);
    graphicsPipeline_ = std::move(other.graphicsPipeline_);
    descriptorManager_ = std::move(other.descriptorManager_);
    commandRecorder_ = std::move(other.commandRecorder_);
    indirectRenderer_ = std::move(other.indirectRenderer_);
    cameraBuffer_ = std::move(other.cameraBuffer_);
    modelBuffer_ = std::move(other.modelBuffer_);
    currentDescriptorSet_ = other.currentDescriptorSet_;
    commandPool_ = other.commandPool_;
    width_ = other.width_;
    height_ = other.height_;
    isInitialized_ = other.isInitialized_;
    inFrame_ = other.inFrame_;

    other.context_ = nullptr;
    other.currentDescriptorSet_ = VK_NULL_HANDLE;
    other.commandPool_ = VK_NULL_HANDLE;
    other.width_ = 0;
    other.height_ = 0;
    other.isInitialized_ = false;
    other.inFrame_ = false;
}

VulkanRenderer3D& VulkanRenderer3D::operator=(VulkanRenderer3D&& other) noexcept {
    if (this != &other) {
        Destroy();

        context_ = other.context_;
        renderPassManager_ = std::move(other.renderPassManager_);
        graphicsPipeline_ = std::move(other.graphicsPipeline_);
        descriptorManager_ = std::move(other.descriptorManager_);
        commandRecorder_ = std::move(other.commandRecorder_);
        indirectRenderer_ = std::move(other.indirectRenderer_);
        cameraBuffer_ = std::move(other.cameraBuffer_);
        modelBuffer_ = std::move(other.modelBuffer_);
        currentDescriptorSet_ = other.currentDescriptorSet_;
        commandPool_ = other.commandPool_;
        width_ = other.width_;
        height_ = other.height_;
        isInitialized_ = other.isInitialized_;
        inFrame_ = other.inFrame_;

        other.context_ = nullptr;
        other.currentDescriptorSet_ = VK_NULL_HANDLE;
        other.commandPool_ = VK_NULL_HANDLE;
        other.width_ = 0;
        other.height_ = 0;
        other.isInitialized_ = false;
        other.inFrame_ = false;
    }
    return *this;
}

bool VulkanRenderer3D::Initialize(VulkanContext* context, uint32_t width, uint32_t height) {
    if (context == nullptr || !context->Ready() || width == 0 || height == 0) {
        return false;
    }

    Destroy();
    context_ = context;
    width_ = width;
    height_ = height;

    VkDevice device = context_->Device();
    VkPhysicalDevice physicalDevice = context_->PhysicalDevice();

    RenderPassConfig rpConfig{};
    rpConfig.colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    rpConfig.enableDepth = false;
    rpConfig.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    rpConfig.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (!renderPassManager_.Initialize(device, rpConfig)) {
        Destroy();
        return false;
    }

    std::vector<DescriptorLayoutBindingInfo> bindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1},
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1}
    };

    if (!descriptorManager_.Initialize(device, bindings, 8)) {
        Destroy();
        return false;
    }

    if (!cameraBuffer_.Initialize(device, physicalDevice, sizeof(CameraUBO), VulkanBufferType::UniformBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ||
        !modelBuffer_.Initialize(device, physicalDevice, sizeof(ModelUBO), VulkanBufferType::UniformBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        Destroy();
        return false;
    }

    currentDescriptorSet_ = descriptorManager_.AllocateSet();
    if (currentDescriptorSet_ != VK_NULL_HANDLE) {
        descriptorManager_.UpdateBufferBinding(currentDescriptorSet_, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, cameraBuffer_.GetBuffer(), 0, sizeof(CameraUBO));
        descriptorManager_.UpdateBufferBinding(currentDescriptorSet_, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, modelBuffer_.GetBuffer(), 0, sizeof(ModelUBO));
    }

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = context_->GraphicsQueueFamily();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    if (!commandRecorder_.Initialize(device, commandPool_) ||
        !indirectRenderer_.Initialize(device, physicalDevice)) {
        Destroy();
        return false;
    }

    isInitialized_ = true;
    return true;
}

bool VulkanRenderer3D::BeginFrame() {
    if (!isInitialized_ || inFrame_) {
        return false;
    }

    if (!commandRecorder_.BeginRecording()) {
        return false;
    }

    inFrame_ = true;
    return true;
}

void VulkanRenderer3D::SetCamera(const CameraUBO& camera) {
    if (!inFrame_) return;
    cameraBuffer_.UploadData(&camera, sizeof(CameraUBO));
}

void VulkanRenderer3D::DrawMesh(const VulkanMeshBufferBuilder& mesh, const ModelUBO& model, const VulkanGPUTexture* /*texture*/) {
    if (!inFrame_ || !mesh.IsValid()) return;

    modelBuffer_.UploadData(&model, sizeof(ModelUBO));

    commandRecorder_.BindVertexBuffer(mesh.GetVertexBuffer().GetBuffer());
    commandRecorder_.BindIndexBuffer(mesh.GetIndexBuffer().GetBuffer());
    if (currentDescriptorSet_ != VK_NULL_HANDLE && graphicsPipeline_.IsValid()) {
        commandRecorder_.BindDescriptorSets(graphicsPipeline_.GetPipelineLayout(), &currentDescriptorSet_, 1);
    }
    commandRecorder_.DrawIndexed(mesh.GetIndexCount());
}

bool VulkanRenderer3D::DrawMeshBatch(const VulkanMeshBatchBuffer& batch) {
    if (!inFrame_ || !batch.IsValid() || batch.MeshCount() > indirectRenderer_.Capacity()) {
        return false;
    }

    commandRecorder_.BindVertexBuffer(batch.GetVertexBuffer().GetBuffer());
    commandRecorder_.BindIndexBuffer(batch.GetIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    for (std::size_t i = 0; i < batch.MeshCount(); ++i) {
        const VulkanMeshBatchRange& range = batch.GetRange(i);
        if (!indirectRenderer_.TrySubmitDraw({range.indexCount, 1, range.firstIndex,
                                               static_cast<int32_t>(range.vertexOffset),
                                               static_cast<uint32_t>(i)})) {
            return false;
        }
    }

    return true;
}

bool VulkanRenderer3D::EndFrame() {
    if (!inFrame_) {
        return false;
    }

    bool ok = true;
    if (indirectRenderer_.PendingDrawCount() > 0) {
        ok = indirectRenderer_.Execute(commandRecorder_.GetCommandBuffer());
    }
    ok = commandRecorder_.EndRecording() && ok;
    inFrame_ = false;
    return ok;
}

void VulkanRenderer3D::Destroy() {
    if (context_ != nullptr && context_->Ready()) {
        VkDevice device = context_->Device();
        commandRecorder_.Destroy();

        if (commandPool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, commandPool_, nullptr);
            commandPool_ = VK_NULL_HANDLE;
        }

        indirectRenderer_.Destroy();
        cameraBuffer_.Destroy();
        modelBuffer_.Destroy();
        descriptorManager_.Destroy();
        graphicsPipeline_.Destroy();
        renderPassManager_.Destroy();
        context_ = nullptr;
    }
    width_ = 0;
    height_ = 0;
    isInitialized_ = false;
    inFrame_ = false;
}

} // namespace NeoEngine
