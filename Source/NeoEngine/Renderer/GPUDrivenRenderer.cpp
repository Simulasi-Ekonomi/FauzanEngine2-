#include "GPUDrivenRenderer.h"

void GPUDrivenRenderer::Initialize(VkDevice device) {
    commands.clear();
    device_ = device;
    initialized_ = device != VK_NULL_HANDLE;
    indirectBuffer = VK_NULL_HANDLE;
}

void GPUDrivenRenderer::SubmitDraw(const GPUIndirectCommand& cmd) {
    (void)TrySubmitDraw(cmd);
}

bool GPUDrivenRenderer::TrySubmitDraw(const GPUIndirectCommand& cmd) {
    if (!initialized_ || cmd.indexCount == 0U || cmd.instanceCount == 0U || commands.size() >= MaxCommands) return false;
    commands.push_back(cmd);
    return true;
}

void GPUDrivenRenderer::Execute(VkCommandBuffer cmdBuffer) {
    if (!initialized_ || cmdBuffer == VK_NULL_HANDLE || indirectBuffer == VK_NULL_HANDLE || commands.empty()) return;
    vkCmdDrawIndexedIndirect(cmdBuffer, indirectBuffer, 0, static_cast<uint32_t>(commands.size()), sizeof(GPUIndirectCommand));
    commands.clear();
}
