#include "GPUDrivenRenderer.h"

void GPUDrivenRenderer::Initialize(VkDevice device)
{
    commands.reserve(10000);
}

void GPUDrivenRenderer::SubmitDraw(const GPUIndirectCommand& cmd)
{
    commands.push_back(cmd);
}

void GPUDrivenRenderer::Execute(VkCommandBuffer cmdBuffer)
{
    if(commands.empty()) return;

    vkCmdDrawIndexedIndirect(
        cmdBuffer,
        indirectBuffer,
        0,
        commands.size(),
        sizeof(GPUIndirectCommand)
    );

    commands.clear();
}
