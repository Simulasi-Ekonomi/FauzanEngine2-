#include "Runtime/VulkanMeshBatchBuffer.h"
#include "Runtime/VulkanRenderCommandRecorder.h"
#include "Renderer/GPUDrivenRenderer.h"

#include <cassert>
#include <cstdint>
#include <vector>

namespace {

NeoEngine::MeshVertex3D Vertex(float x, float y, float z) {
    return {{x, y, z}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}};
}

} // namespace

int main() {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "FauzanEngine2 R3 shared mesh indirect smoke";
    app.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &app;

    VkInstance instance = VK_NULL_HANDLE;
    assert(vkCreateInstance(&instanceInfo, nullptr, &instance) == VK_SUCCESS);

    uint32_t physicalCount = 0;
    assert(vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr) == VK_SUCCESS);
    assert(physicalCount > 0);
    std::vector<VkPhysicalDevice> physicalDevices(physicalCount);
    assert(vkEnumeratePhysicalDevices(instance, &physicalCount, physicalDevices.data()) == VK_SUCCESS);

    VkPhysicalDevice physical = physicalDevices.front();
    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &familyCount, families.data());

    uint32_t graphicsFamily = UINT32_MAX;
    for (uint32_t i = 0; i < familyCount; ++i) {
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
            graphicsFamily = i;
            break;
        }
    }
    assert(graphicsFamily != UINT32_MAX);

    float priority = 1.0F;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = graphicsFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;

    VkDevice device = VK_NULL_HANDLE;
    assert(vkCreateDevice(physical, &deviceInfo, nullptr, &device) == VK_SUCCESS);

    NeoEngine::VulkanMeshBufferBuilder meshA;
    NeoEngine::VulkanMeshBufferBuilder meshB;
    const std::vector<NeoEngine::MeshVertex3D> verticesA = {
        Vertex(-1.0F, -1.0F, 0.0F), Vertex(1.0F, -1.0F, 0.0F), Vertex(0.0F, 1.0F, 0.0F)};
    const std::vector<NeoEngine::MeshVertex3D> verticesB = {
        Vertex(-0.5F, -0.5F, 0.0F), Vertex(0.5F, -0.5F, 0.0F), Vertex(0.0F, 0.5F, 0.0F)};
    const std::vector<uint32_t> indices = {0, 1, 2};
    assert(meshA.BuildMesh(device, physical, verticesA, indices));
    assert(meshB.BuildMesh(device, physical, verticesB, indices));

    NeoEngine::VulkanMeshBatchBuffer batch;
    const std::vector<const NeoEngine::VulkanMeshBufferBuilder*> meshes = {&meshA, &meshB};
    assert(batch.Build(device, physical, meshes));
    assert(batch.MeshCount() == 2);
    assert(batch.GetRange(0).vertexOffset == 0);
    assert(batch.GetRange(0).firstIndex == 0);
    assert(batch.GetRange(0).indexCount == 3);
    assert(batch.GetRange(1).vertexOffset == 3);
    assert(batch.GetRange(1).firstIndex == 3);
    assert(batch.GetRange(1).indexCount == 3);

    GPUDrivenRenderer indirect;
    assert(indirect.Initialize(device, physical, 2));
    assert(indirect.TrySubmitDraw({batch.GetRange(0).indexCount, 1, batch.GetRange(0).firstIndex,
                                   static_cast<int32_t>(batch.GetRange(0).vertexOffset), 0}));
    assert(indirect.TrySubmitDraw({batch.GetRange(1).indexCount, 1, batch.GetRange(1).firstIndex,
                                   static_cast<int32_t>(batch.GetRange(1).vertexOffset), 1}));
    assert(indirect.PendingDrawCount() == 2);

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool pool = VK_NULL_HANDLE;
    assert(vkCreateCommandPool(device, &poolInfo, nullptr, &pool) == VK_SUCCESS);

    NeoEngine::VulkanRenderCommandRecorder recorder;
    assert(recorder.Initialize(device, pool));
    assert(recorder.BeginRecording());
    recorder.BindVertexBuffer(batch.GetVertexBuffer().GetBuffer());
    recorder.BindIndexBuffer(batch.GetIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    assert(indirect.Execute(recorder.GetCommandBuffer()));
    assert(indirect.PendingDrawCount() == 0);
    assert(recorder.EndRecording());

    indirect.Destroy();
    recorder.Destroy();
    vkDestroyCommandPool(device, pool, nullptr);
    batch.Destroy();
    meshA.Destroy();
    meshB.Destroy();
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    return 0;
}
