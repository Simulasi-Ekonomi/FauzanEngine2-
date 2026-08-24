#pragma once
#include <vector>
#include <vulkan/vulkan.h>

struct GPUIndirectCommand
{
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t vertexOffset;
    uint32_t firstInstance;
};

class GPUDrivenRenderer
{
public:

    void Initialize(VkDevice device);

    void SubmitDraw(const GPUIndirectCommand& cmd);

    void Execute(VkCommandBuffer cmdBuffer);

private:

    std::vector<GPUIndirectCommand> commands;
    VkBuffer indirectBuffer{};
};
