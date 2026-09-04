#include "Runtime/VulkanContext.h"
#include "Runtime/VulkanGPUBuffer.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

struct TestVertex {
    float pos[3];
    float uv[2];
};

struct TestUniform {
    float mvp[16];
};

int main() {
    std::cout << "[Smoke Test] Starting vulkan_gpu_buffer_smoke...\n";

    NeoEngine::VulkanContext context;
    if (!context.Initialize()) {
        std::cerr << "Failed to initialize VulkanContext\n";
        return 1;
    }

    VkDevice device = context.Device();
    VkPhysicalDevice physicalDevice = context.PhysicalDevice();

    // 1. Test Vertex Buffer
    std::vector<TestVertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.0f,  0.5f, 0.0f}, {0.5f, 1.0f}}
    };
    VkDeviceSize vertexSize = sizeof(TestVertex) * vertices.size();

    NeoEngine::VulkanGPUBuffer vertexBuffer;
    bool vInit = vertexBuffer.Initialize(device, physicalDevice, vertexSize,
                                         NeoEngine::VulkanBufferType::VertexBuffer,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    assert(vInit && "VertexBuffer initialization failed");
    assert(vertexBuffer.IsValid());
    assert(vertexBuffer.GetSize() == vertexSize);
    assert(vertexBuffer.GetType() == NeoEngine::VulkanBufferType::VertexBuffer);

    bool vUpload = vertexBuffer.UploadData(vertices.data(), vertexSize);
    assert(vUpload && "VertexBuffer upload failed");

    std::vector<TestVertex> readVertices(vertices.size());
    bool vRead = vertexBuffer.ReadData(readVertices.data(), vertexSize);
    assert(vRead && "VertexBuffer read failed");
    assert(std::memcmp(vertices.data(), readVertices.data(), vertexSize) == 0 && "VertexBuffer content mismatch");

    // 2. Test Index Buffer
    std::vector<uint32_t> indices = {0, 1, 2};
    VkDeviceSize indexSize = sizeof(uint32_t) * indices.size();

    NeoEngine::VulkanGPUBuffer indexBuffer;
    bool iInit = indexBuffer.Initialize(device, physicalDevice, indexSize,
                                        NeoEngine::VulkanBufferType::IndexBuffer,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    assert(iInit && "IndexBuffer initialization failed");
    assert(indexBuffer.IsValid());

    bool iUpload = indexBuffer.UploadData(indices.data(), indexSize);
    assert(iUpload && "IndexBuffer upload failed");

    std::vector<uint32_t> readIndices(indices.size());
    bool iRead = indexBuffer.ReadData(readIndices.data(), indexSize);
    assert(iRead && "IndexBuffer read failed");
    assert(indices == readIndices && "IndexBuffer content mismatch");

    // 3. Test Uniform Buffer
    TestUniform uboData{};
    for (int i = 0; i < 16; ++i) {
        uboData.mvp[i] = static_cast<float>(i + 1);
    }
    VkDeviceSize uboSize = sizeof(TestUniform);

    NeoEngine::VulkanGPUBuffer uniformBuffer;
    bool uInit = uniformBuffer.Initialize(device, physicalDevice, uboSize,
                                          NeoEngine::VulkanBufferType::UniformBuffer,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    assert(uInit && "UniformBuffer initialization failed");
    assert(uniformBuffer.IsValid());

    bool uUpload = uniformBuffer.UploadData(&uboData, uboSize);
    assert(uUpload && "UniformBuffer upload failed");

    TestUniform readUbo{};
    bool uRead = uniformBuffer.ReadData(&readUbo, uboSize);
    assert(uRead && "UniformBuffer read failed");
    assert(std::memcmp(&uboData, &readUbo, uboSize) == 0 && "UniformBuffer content mismatch");

    // 4. Test Move Semantics
    NeoEngine::VulkanGPUBuffer movedBuffer = std::move(vertexBuffer);
    assert(!vertexBuffer.IsValid() && "Moved-from buffer should be invalid");
    assert(movedBuffer.IsValid() && "Moved-to buffer should be valid");

    std::vector<TestVertex> readMovedVertices(vertices.size());
    bool mRead = movedBuffer.ReadData(readMovedVertices.data(), vertexSize);
    assert(mRead && "MovedBuffer read failed");
    assert(std::memcmp(vertices.data(), readMovedVertices.data(), vertexSize) == 0);

    // RAII Cleanup
    movedBuffer.Destroy();
    indexBuffer.Destroy();
    uniformBuffer.Destroy();
    context.Reset();

    std::cout << "[Smoke Test] vulkan_gpu_buffer_smoke passed successfully!\n";
    return 0;
}
