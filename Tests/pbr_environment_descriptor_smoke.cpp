#include "Runtime/PBREnvironmentDescriptorSet.h"
#include "Runtime/VulkanContext.h"

#include <cassert>
#include <iostream>

int main() {
    NeoEngine::VulkanContext context;
    if (!context.Initialize()) {
        std::cerr << "Vulkan context unavailable\n";
        return 1;
    }

    NeoEngine::PBREnvironmentDescriptorSet descriptors;
    assert(descriptors.Initialize(context.Device(), 1));
    assert(descriptors.IsValid());
    assert(descriptors.GetLayout() != VK_NULL_HANDLE);

    const VkDescriptorSet set = descriptors.AllocateSet();
    assert(set != VK_NULL_HANDLE);

    descriptors.Destroy();
    context.Reset();
    std::cout << "PBR environment descriptor smoke: PASS\n";
    return 0;
}
