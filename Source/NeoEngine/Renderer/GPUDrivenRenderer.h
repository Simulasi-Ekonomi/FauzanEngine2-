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

    // Compatibility initializer: preserves the original API. It enables CPU-side
    // command collection but cannot create a GPU indirect buffer without a
    // physical device. Use the overload below for actual GPU execution.
    void Initialize(VkDevice device);

    // R3 production path: creates a host-visible/coherent indirect command
    // buffer sized for the device's supported draw-indirect limit.
    bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice,
                    std::size_t maxCommands = MaxCommands);

    void Destroy();

    void SubmitDraw(const GPUIndirectCommand& cmd);
    bool TrySubmitDraw(const GPUIndirectCommand& cmd);

    // Uploads the pending command list and records one vkCmdDrawIndexedIndirect.
    bool Execute(VkCommandBuffer cmdBuffer);

    [[nodiscard]] std::size_t PendingDrawCount() const { return commands.size(); }
    [[nodiscard]] std::size_t Capacity() const { return maxCommands_; }
    [[nodiscard]] bool IsInitialized() const { return initialized_; }
    [[nodiscard]] bool HasGpuBuffer() const { return indirectBuffer != VK_NULL_HANDLE; }
    [[nodiscard]] VkBuffer IndirectBuffer() const { return indirectBuffer; }

private:
    static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice,
                                   uint32_t typeFilter,
                                   VkMemoryPropertyFlags properties);

    std::vector<GPUIndirectCommand> commands;
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkBuffer indirectBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indirectMemory = VK_NULL_HANDLE;
    std::size_t maxCommands_ = 0;
    bool initialized_ = false;
};
