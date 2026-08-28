#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

struct GPUIndirectCommand {
    uint32_t indexCount{};
    uint32_t instanceCount{};
    uint32_t firstIndex{};
    int32_t vertexOffset{};
    uint32_t firstInstance{};
};

class GPUDrivenRenderer {
public:
    static constexpr std::size_t MaxCommands = 10000;
    void Initialize(VkDevice device);
    void SubmitDraw(const GPUIndirectCommand& cmd);
    bool TrySubmitDraw(const GPUIndirectCommand& cmd);
    void Execute(VkCommandBuffer cmdBuffer);
    [[nodiscard]] std::size_t PendingDrawCount() const { return commands.size(); }
    [[nodiscard]] bool IsInitialized() const { return initialized_; }
private:
    std::vector<GPUIndirectCommand> commands;
    VkDevice device_ = VK_NULL_HANDLE;
    VkBuffer indirectBuffer = VK_NULL_HANDLE;
    bool initialized_ = false;
};
