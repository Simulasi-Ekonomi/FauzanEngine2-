#include "Runtime/VulkanContext.h"
#include "Runtime/VulkanRenderer3D.h"
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
    std::cout << "[Smoke Test] Starting vulkan_renderer_3d_smoke...\n";

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

    // 1. Initialize VulkanRenderer3D
    NeoEngine::VulkanRenderer3D renderer;
    bool init = renderer.Initialize(&context, 800, 600);
    TEST_CHECK(init, "VulkanRenderer3D initialization failed");
    TEST_CHECK(renderer.IsInitialized(), "IsInitialized check failed");

    // 2. Prepare 3D Mesh
    std::vector<NeoEngine::MeshVertex3D> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}}
    };
    std::vector<uint32_t> indices = {0, 1, 2};

    NeoEngine::VulkanMeshBufferBuilder mesh;
    bool mInit = mesh.BuildMesh(device, physicalDevice, vertices, indices);
    TEST_CHECK(mInit, "Mesh build failed");

    // 3. Render Loop Simulation
    bool bFrame = renderer.BeginFrame();
    TEST_CHECK(bFrame, "BeginFrame failed");

    NeoEngine::CameraUBO camera{};
    renderer.SetCamera(camera);

    NeoEngine::ModelUBO model{};
    renderer.DrawMesh(mesh, model);

    bool eFrame = renderer.EndFrame();
    TEST_CHECK(eFrame, "EndFrame failed");

    // 4. Test Move Semantics
    NeoEngine::VulkanRenderer3D movedRenderer = std::move(renderer);
    TEST_CHECK(!renderer.IsInitialized(), "Moved-from renderer should be uninitialized");
    TEST_CHECK(movedRenderer.IsInitialized(), "Moved-to renderer should be initialized");

    // RAII Cleanup
    mesh.Destroy();
    movedRenderer.Destroy();
    context.Reset();

    std::cout << "[Smoke Test] vulkan_renderer_3d_smoke passed successfully!\n";
    return 0;
}
