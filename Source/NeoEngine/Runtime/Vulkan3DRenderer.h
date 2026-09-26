#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>
#include <string>
#include "Animation/GPUSkinningPaletteBuffer.h"
#include "VulkanAssetUploader.h"

#include <vulkan/vulkan.h>

namespace NeoEngine {

class RuntimeAssetStreamBridge;
class VulkanGPUTexture;

struct Vulkan3DVertex {
    float px = 0.0F, py = 0.0F, pz = 0.0F;
    float nx = 0.0F, ny = 0.0F, nz = 1.0F;
    float u = 0.0F, v = 0.0F;
    std::array<uint32_t, 4> boneIndices{};
    std::array<float, 4> boneWeights{};
    std::array<float, 4> materialColor{1.0F, 1.0F, 1.0F, 1.0F};
};

struct Vulkan3DFrameStats {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint64_t frameIndex = 0;
};

enum class Vulkan3DRendererError : uint8_t {
    None,
    InvalidConfiguration,
    SdlFailure,
    VulkanFailure,
    SwapchainOutOfDate,
    DeviceLost,
    ShaderUnavailable,
    PipelineFailure,
    BufferFailure,
    FrameFailure
};

class Vulkan3DRenderer {
public:
    Vulkan3DRenderer() = default;
    ~Vulkan3DRenderer();
    Vulkan3DRenderer(const Vulkan3DRenderer&) = delete;
    Vulkan3DRenderer& operator=(const Vulkan3DRenderer&) = delete;

    bool Initialize(uint32_t width, uint32_t height, const char* title = "NeoEngine 3D");
    bool Resize(uint32_t width, uint32_t height);
    bool BeginFrame(float clearR = 0.05F, float clearG = 0.05F, float clearB = 0.07F, float clearA = 1.0F);
    bool DrawIndexed(std::span<const Vulkan3DVertex> vertices, std::span<const uint32_t> indices,
                     const float* modelViewProjection4x4);
    bool DrawIndexedSkinned(std::span<const Vulkan3DVertex> vertices, std::span<const uint32_t> indices,
                            const float* modelViewProjection4x4, const float* model4x4);

    // R3: uploads one mesh once and renders it with N GPU instance transforms in one draw call.
    // Matrices are contiguous row-major 4x4 transforms (16 floats each).
    // The original DrawIndexed API remains unchanged.
    bool DrawIndexedInstanced(std::span<const Vulkan3DVertex> vertices,
                              std::span<const uint32_t> indices,
                              std::span<const float> modelViewProjections4x4);

    bool EndFrame();
    // Forces tracked asset uploads to observe their authoritative completion state
    // while the Vulkan device is still valid. Call before destroying the renderer
    // and before releasing runtime-owned streamed GPU resources.
    void FlushAssetUploads() noexcept;
    // Binds the runtime bridge as the sole completion/ownership publication path.
    bool BindAssetStreamBridge(RuntimeAssetStreamBridge& bridge) noexcept;
    // Records decoded streamed texture uploads into the active frame command buffer.
    bool PumpAssetStreamUploads(RuntimeAssetStreamBridge& bridge) noexcept;
    bool PumpAssetStreamUploads() noexcept;
    // Binds a completed streamed texture for subsequent draws in the active frame.
    // The descriptor is immutable for the lifetime of the renderer-owned streamed resource.
    bool BindStreamedTexture(const std::string& assetId) noexcept;
    [[nodiscard]] bool IsStreamedTextureReady(const std::string& assetId) const noexcept;
    [[nodiscard]] const VulkanGPUTexture* FindStreamedTexture(const std::string& assetId) const noexcept;

    // Copies the last successfully presented swapchain image into RGBA8 CPU memory.
    // Must be called after EndFrame() and before the next BeginFrame().
    bool ReadbackLastFrame(std::vector<uint8_t>& rgba8);
    void Reset();

    // Resource-owned GPU upload path; valid only while a frame is active.
    // Upload completion is bound internally to the submitted frame fence; the uploader
    // is advanced after that fence is waited and before the fence is reset for reuse.
    bool UploadTextureResource(AssetResourceManager& resources, const AssetResourceHandle& handle,
                               VkImage targetImage, VkImageLayout targetLayout,
                               uint32_t width, uint32_t height);
    bool UploadSkinningPalette(const std::vector<Mat4>& palette);
    bool UseDefaultSkinningPalette();

    [[nodiscard]] bool Ready() const { return ready_; }
    [[nodiscard]] Vulkan3DRendererError LastError() const { return lastError_; }
    [[nodiscard]] const Vulkan3DFrameStats& LastFrameStats() const { return stats_; }

private:
    struct Impl;
    Impl* impl_ = nullptr;
    bool ready_ = false;
    Vulkan3DRendererError lastError_ = Vulkan3DRendererError::None;
    Vulkan3DFrameStats stats_{};
};

} // namespace NeoEngine
