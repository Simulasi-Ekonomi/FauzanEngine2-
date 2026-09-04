#include "Runtime/VulkanContext.h"
#include "Runtime/VulkanGPUBuffer.h"
#include <cstring>
#include <iostream>
#include <vector>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\n"; \
            return 1; \
        } \
    } while (0)

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
        std::cout << "[INFO] VulkanContext failed to initialize (likely headless environment). Skipping hardware test gracefully.\n";
        return 0;
    }

    VkDevice device = context.Device();
    VkPhysicalDevice physicalDevice = context.PhysicalDevice();

    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE) {
        std::cout << "[INFO] No valid Vulkan physical or logical device available. Skipping test gracefully.\n";
        return 0;
    }

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
    TEST_CHECK(vInit, "VertexBuffer initialization failed");
    TEST_CHECK(vertexBuffer.IsValid(), "VertexBuffer IsValid check failed");
    TEST_CHECK(vertexBuffer.GetSize() == vertexSize, "VertexBuffer size mismatch");
    TEST_CHECK(vertexBuffer.GetType() == NeoEngine::VulkanBufferType::VertexBuffer, "VertexBuffer type mismatch");

    bool vUpload = vertexBuffer.UploadData(vertices.data(), vertexSize);
    TEST_CHECK(vUpload, "VertexBuffer upload failed");

    std::vector<TestVertex> readVertices(vertices.size());
    bool vRead = vertexBuffer.ReadData(readVertices.data(), vertexSize);
    TEST_CHECK(vRead, "VertexBuffer read failed");
    TEST_CHECK(std::memcmp(vertices.data(), readVertices.data(), vertexSize) == 0, "VertexBuffer content mismatch");

    // 2. Test Index Buffer
    std::vector<uint32_t> indices = {0, 1, 2};
    VkDeviceSize indexSize = sizeof(uint32_t) * indices.size();

    NeoEngine::VulkanGPUBuffer indexBuffer;
    bool iInit = indexBuffer.Initialize(device, physicalDevice, indexSize,
                                        NeoEngine::VulkanBufferType::IndexBuffer,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    TEST_CHECK(iInit, "IndexBuffer initialization failed");
    TEST_CHECK(indexBuffer.IsValid(), "IndexBuffer IsValid check failed");

    bool iUpload = indexBuffer.UploadData(indices.data(), indexSize);
    TEST_CHECK(iUpload, "IndexBuffer upload failed");

    std::vector<uint32_t> readIndices(indices.size());
    bool iRead = indexBuffer.ReadData(readIndices.data(), indexSize);
    TEST_CHECK(iRead, "IndexBuffer read failed");
    TEST_CHECK(indices == readIndices, "IndexBuffer content mismatch");

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
    TEST_CHECK(uInit, "UniformBuffer initialization failed");
    TEST_CHECK(uniformBuffer.IsValid(), "UniformBuffer IsValid check failed");

    bool uUpload = uniformBuffer.UploadData(&uboData, uboSize);
    TEST_CHECK(uUpload, "UniformBuffer upload failed");

    TestUniform readUbo{};
    bool uRead = uniformBuffer.ReadData(&readUbo, uboSize);
    TEST_CHECK(uRead, "UniformBuffer read failed");
    TEST_CHECK(std::memcmp(&uboData, &readUbo, uboSize) == 0, "UniformBuffer content mismatch");

    // 4. Test Move Semantics
    NeoEngine::VulkanGPUBuffer movedBuffer = std::move(vertexBuffer);
    TEST_CHECK(!vertexBuffer.IsValid(), "Moved-from buffer should be invalid");
    TEST_CHECK(movedBuffer.IsValid(), "Moved-to buffer should be valid");

    std::vector<TestVertex> readMovedVertices(vertices.size());
    bool mRead = movedBuffer.ReadData(readMovedVertices.data(), vertexSize);
    TEST_CHECK(mRead, "MovedBuffer read failed");
    TEST_CHECK(std::memcmp(vertices.data(), readMovedVertices.data(), vertexSize) == 0, "MovedBuffer content mismatch");

    // RAII Cleanup
    movedBuffer.Destroy();
    indexBuffer.Destroy();
    uniformBuffer.Destroy();
    context.Reset();

    std::cout << "[Smoke Test] vulkan_gpu_buffer_smoke passed successfully!\n";
    return 0;
}
