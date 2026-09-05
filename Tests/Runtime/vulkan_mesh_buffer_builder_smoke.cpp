#include "Runtime/VulkanContext.h"
#include "Runtime/VulkanMeshBufferBuilder.h"
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
    std::cout << "[Smoke Test] Starting vulkan_mesh_buffer_builder_smoke...\n";

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

    // 1. Define 3D Quad/Cube Mesh Data
    std::vector<NeoEngine::MeshVertex3D> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
    };

    std::vector<uint32_t> indices = {
        0, 1, 2,
        2, 3, 0
    };

    // 2. Build Mesh Buffers
    NeoEngine::VulkanMeshBufferBuilder builder;
    bool built = builder.BuildMesh(device, physicalDevice, vertices, indices);
    TEST_CHECK(built, "Mesh building failed");
    TEST_CHECK(builder.IsValid(), "Builder IsValid check failed");
    TEST_CHECK(builder.GetVertexCount() == 4, "Vertex count mismatch");
    TEST_CHECK(builder.GetIndexCount() == 6, "Index count mismatch");
    TEST_CHECK(builder.GetVertexBuffer().IsValid(), "VertexBuffer check failed");
    TEST_CHECK(builder.GetIndexBuffer().IsValid(), "IndexBuffer check failed");

    // 3. Test Move Semantics
    NeoEngine::VulkanMeshBufferBuilder movedBuilder = std::move(builder);
    TEST_CHECK(!builder.IsValid(), "Moved-from builder should be invalid");
    TEST_CHECK(movedBuilder.IsValid(), "Moved-to builder should be valid");
    TEST_CHECK(movedBuilder.GetIndexCount() == 6, "Moved-to index count mismatch");

    // RAII Cleanup
    movedBuilder.Destroy();
    context.Reset();

    std::cout << "[Smoke Test] vulkan_mesh_buffer_builder_smoke passed successfully!\n";
    return 0;
}
