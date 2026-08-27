#include "Runtime/VulkanTexturedPresent.h"

#include <cstdint>
#include <cstdio>
#include <vulkan/vulkan.h>

int main() {
    using namespace NeoEngine;
    if (VulkanTexturedPresentProbe::ClassifyDriverResult(static_cast<int32_t>(VK_SUCCESS)) != VulkanPresentStatus::None ||
        VulkanTexturedPresentProbe::ClassifyDriverResult(static_cast<int32_t>(VK_SUBOPTIMAL_KHR)) != VulkanPresentStatus::None ||
        VulkanTexturedPresentProbe::ClassifyDriverResult(static_cast<int32_t>(VK_ERROR_DEVICE_LOST)) != VulkanPresentStatus::DeviceLost ||
        VulkanTexturedPresentProbe::ClassifyDriverResult(static_cast<int32_t>(VK_ERROR_OUT_OF_DATE_KHR)) != VulkanPresentStatus::SurfaceOutOfDate ||
        VulkanTexturedPresentProbe::ClassifyDriverResult(static_cast<int32_t>(VK_TIMEOUT)) != VulkanPresentStatus::Timeout ||
        VulkanTexturedPresentProbe::ClassifyDriverResult(-987654321) != VulkanPresentStatus::DriverRejected) return 1;
    std::printf("VULKAN_PRESENT_STATUS_SMOKE_OK success=1 suboptimal=1 device_lost=1 out_of_date=1 timeout=1 rejected=1\n");
    return 0;
}
