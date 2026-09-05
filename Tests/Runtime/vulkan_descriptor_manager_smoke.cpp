#include "Runtime/VulkanContext.h"
#include "Runtime/VulkanDescriptorManager.h"
#include "Runtime/VulkanGPUBuffer.h"
#include "Runtime/VulkanGPUTexture.h"
#include <iostream>
#include <vector>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\n"; \
            return 1; \
        } \
    } while (0)

int main() {
    std::cout << "[Smoke Test] Starting vulkan_descriptor_manager_smoke...\n";

    NeoEngine::VulkanContext context;
    if (!context.Initialize()) {
        std::cout << "[INFO] VulkanContext failed to initialize (likely headless environment). Skipping hardware test gracefully.\n";
        return 0;
    }

    VkDevice device = context.Device();
    VkPhysicalDevice physicalDevice = context.PhysicalDevice();

    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE) {
        std::cout << "[INFO] No valid Vulkan physical or logical device available. Skipping test gracefully.\n";
        return 0;
    }

    // 1. Define Bindings (Binding 0: Uniform Buffer, Binding 1: Combined Image Sampler)
    std::vector<NeoEngine::DescriptorLayoutBindingInfo> bindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1}
    };

    NeoEngine::VulkanDescriptorManager manager;
    bool init = manager.Initialize(device, bindings, 4);
    TEST_CHECK(init, "DescriptorManager initialization failed");
    TEST_CHECK(manager.IsValid(), "DescriptorManager IsValid check failed");
    TEST_CHECK(manager.GetLayout() != VK_NULL_HANDLE, "GetLayout returned NULL");
    TEST_CHECK(manager.GetPool() != VK_NULL_HANDLE, "GetPool returned NULL");

    // 2. Allocate Descriptor Set
    VkDescriptorSet descriptorSet = manager.AllocateSet();
    TEST_CHECK(descriptorSet != VK_NULL_HANDLE, "AllocateSet returned NULL");

    // 3. Create UBO Buffer and Update Binding
    NeoEngine::VulkanGPUBuffer uboBuffer;
    bool uInit = uboBuffer.Initialize(device, physicalDevice, 64, NeoEngine::VulkanBufferType::UniformBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    TEST_CHECK(uInit, "UBO Buffer initialization failed");

    manager.UpdateBufferBinding(descriptorSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uboBuffer.GetBuffer(), 0, uboBuffer.GetSize());

    // 4. Create Texture + Sampler and Update Binding
    NeoEngine::VulkanGPUTexture texture;
    bool tInit = texture.Initialize(device, physicalDevice, 2, 2, VK_FORMAT_R8G8B8A8_UNORM);
    TEST_CHECK(tInit, "Texture initialization failed");

    bool sInit = texture.CreateSampler();
    TEST_CHECK(sInit, "Sampler creation failed");

    manager.UpdateImageBinding(descriptorSet, 1, texture.GetImageView(), texture.GetSampler());

    // 5. Test Move Semantics
    NeoEngine::VulkanDescriptorManager movedManager = std::move(manager);
    TEST_CHECK(!manager.IsValid(), "Moved-from manager should be invalid");
    TEST_CHECK(movedManager.IsValid(), "Moved-to manager should be valid");

    // RAII Teardown
    texture.Destroy();
    uboBuffer.Destroy();
    movedManager.Destroy();
    context.Reset();

    std::cout << "[Smoke Test] vulkan_descriptor_manager_smoke passed successfully!\n";
    return 0;
}
