#pragma once
#include <cstddef>
#include <cstdint>

namespace NeoEngine {

struct RHIBuffer {
    uint32_t handle = 0;
    std::size_t size = 0;
};

struct RHIMesh {
    RHIBuffer vertexBuffer;
    RHIBuffer indexBuffer;
    uint32_t indexCount = 0;
};

class RHI {
public:
    static RHI& Get();
    bool Initialize();
    void Shutdown();

    RHIBuffer CreateVertexBuffer(const void* data, std::size_t size);
    RHIBuffer CreateIndexBuffer(const void* data, std::size_t size);
    void DestroyBuffer(RHIBuffer& buffer);

    void BeginFrame();
    void DrawIndexed(const RHIMesh& mesh, uint32_t instanceCount);
    void EndFrame();

private:
    RHI() = default;
    bool initialized_ = false;
};

} // namespace
