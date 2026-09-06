#pragma once

#include "Runtime/VulkanContext.h"
#include "Runtime/VulkanGPUBuffer.h"
#include "Runtime/VulkanGPUTexture.h"
#include "Runtime/VulkanGraphicsPipeline.h"
#include "Runtime/VulkanDescriptorManager.h"
#include "Runtime/VulkanMeshBufferBuilder.h"
#include "Runtime/VulkanMeshBatchBuffer.h"
#include "Runtime/VulkanRenderPassManager.h"
#include "Runtime/VulkanRenderCommandRecorder.h"
#include "Runtime/VulkanSwapchainManager.h"
#include "Runtime/VulkanSyncPrimitives.h"
#include "Renderer/GPUDrivenRenderer.h"
#include "Renderer/GPUFrustumCulling.h"

#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace NeoEngine {

struct CameraUBO { float viewProjection[16]; };
struct ModelUBO { float model[16]; };

// R3 instance stream matches neo_mesh.vert locations 3..6.
struct VulkanMeshInstanceData {
    float model[16];
};
static_assert(sizeof(VulkanMeshInstanceData) == sizeof(float) * 16);

class VulkanRenderer3D {
public:
    VulkanRenderer3D() = default;
    ~VulkanRenderer3D();

    VulkanRenderer3D(const VulkanRenderer3D&) = delete;
    VulkanRenderer3D& operator=(const VulkanRenderer3D&) = delete;

    VulkanRenderer3D(VulkanRenderer3D&& other) noexcept;
    VulkanRenderer3D& operator=(VulkanRenderer3D&& other) noexcept;

    bool Initialize(VulkanContext* context, uint32_t width, uint32_t height);

    bool BeginFrame();
    void SetCamera(const CameraUBO& camera);
    void DrawMesh(const VulkanMeshBufferBuilder& mesh, const ModelUBO& model, const VulkanGPUTexture* texture = nullptr);

    // R3 path: one shared geometry arena, one instance stream, and one indirect draw.
    // Existing DrawMesh remains unchanged. Each batch entry defaults to identity.
    bool DrawMeshBatch(const VulkanMeshBatchBuffer& batch);

    // R4 path: scene-wide conservative sphere culling against the current camera.
    // Bounds are world-space and correspond one-to-one with batch meshes. The existing
    // DrawMeshBatch API remains unchanged and continues to draw every batch entry.
    bool DrawMeshBatch(const VulkanMeshBatchBuffer& batch,
                       const std::vector<BoundingSphere>& worldBounds);

    bool EndFrame();
    void Destroy();

    [[nodiscard]] bool IsInitialized() const { return isInitialized_; }
    [[nodiscard]] uint32_t GetWidth() const { return width_; }
    [[nodiscard]] uint32_t GetHeight() const { return height_; }

private:
    static constexpr std::size_t MaxBatchInstances = GPUDrivenRenderer::MaxCommands;

    VulkanContext* context_ = nullptr;
    VulkanRenderPassManager renderPassManager_;
    VulkanGraphicsPipeline graphicsPipeline_;
    VulkanDescriptorManager descriptorManager_;
    VulkanRenderCommandRecorder commandRecorder_;
    GPUDrivenRenderer indirectRenderer_;
    GPUFrustumCulling frustumCulling_;

    VulkanGPUBuffer cameraBuffer_;
    VulkanGPUBuffer modelBuffer_;
    VulkanGPUBuffer batchInstanceBuffer_;
    VkDescriptorSet currentDescriptorSet_ = VK_NULL_HANDLE;

    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    bool isInitialized_ = false;
    bool inFrame_ = false;
};

} // namespace NeoEngine
