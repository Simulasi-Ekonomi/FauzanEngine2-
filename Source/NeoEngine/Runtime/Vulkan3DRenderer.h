#pragma once

#include <cstdint>
#include <span>

#include <vulkan/vulkan.h>

namespace NeoEngine {

struct Vulkan3DVertex {
    float px = 0.0F, py = 0.0F, pz = 0.0F;
    float nx = 0.0F, ny = 0.0F, nz = 1.0F;
    float u = 0.0F, v = 0.0F;
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
    bool EndFrame();
    void Reset();

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
