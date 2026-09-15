#include "Runtime/PBREnvironmentDescriptorSet.h"

namespace NeoEngine {

bool PBREnvironmentDescriptorSet::Initialize(VkDevice device, uint32_t maxSets) {
    return descriptors_.Initialize(device,
        {
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1}
        }, maxSets);
}

VkDescriptorSet PBREnvironmentDescriptorSet::AllocateSet() {
    return descriptors_.AllocateSet();
}

void PBREnvironmentDescriptorSet::Update(VkDescriptorSet descriptorSet,
                                         const PBREnvironment& environment,
                                         VkImageView brdfLutView,
                                         VkSampler brdfLutSampler) {
    descriptors_.UpdateImageBinding(descriptorSet, 0,
                                    environment.IrradianceView(), environment.IrradianceSampler());
    descriptors_.UpdateImageBinding(descriptorSet, 1,
                                    environment.PrefilteredView(), environment.PrefilteredSampler());
    descriptors_.UpdateImageBinding(descriptorSet, 2,
                                    brdfLutView, brdfLutSampler);
}

void PBREnvironmentDescriptorSet::Destroy() {
    descriptors_.Destroy();
}

} // namespace NeoEngine
