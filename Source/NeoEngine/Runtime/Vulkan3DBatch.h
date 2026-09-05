#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace NeoEngine {

struct Vulkan3DBatchRange {
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t instanceCount = 0;
    uint32_t firstVertex = 0;
    uint32_t firstIndex = 0;
    uint32_t firstInstance = 0;
};

struct Vulkan3DIndirectCommand {
    uint32_t indexCount = 0;
    uint32_t instanceCount = 0;
    uint32_t firstIndex = 0;
    int32_t vertexOffset = 0;
    uint32_t firstInstance = 0;
};

class Vulkan3DBatchBuilder final {
public:
    void Reset() noexcept;

    bool Append(uint32_t vertexCount, uint32_t indexCount, uint32_t instanceCount) noexcept;

    [[nodiscard]] std::span<const Vulkan3DIndirectCommand> Commands() const noexcept;
    [[nodiscard]] std::span<const Vulkan3DBatchRange> Ranges() const noexcept;

    [[nodiscard]] uint32_t VertexCount() const noexcept { return vertexCount_; }
    [[nodiscard]] uint32_t IndexCount() const noexcept { return indexCount_; }
    [[nodiscard]] uint32_t InstanceCount() const noexcept { return instanceCount_; }

private:
    std::vector<Vulkan3DIndirectCommand> commands_;
    std::vector<Vulkan3DBatchRange> ranges_;
    uint32_t vertexCount_ = 0;
    uint32_t indexCount_ = 0;
    uint32_t instanceCount_ = 0;
};

} // namespace NeoEngine
