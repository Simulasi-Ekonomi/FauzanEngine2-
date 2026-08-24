#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>

namespace NeoEngine {

class GPUComputeSystem {

private:

    VkDevice device;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

public:

    void Dispatch(VkCommandBuffer cmd, uint32_t entityCount, float dt)
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

        vkCmdPushConstants(cmd, pipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0, sizeof(float), &dt);

        uint32_t groupCount = (entityCount + 255) / 256;

        vkCmdDispatch(cmd, groupCount, 1, 1);
    }

};

}
