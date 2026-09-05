#include "Runtime/VulkanContext.h"
#include "Runtime/VulkanGraphicsPipeline.h"
#include <fstream>
#include <iostream>
#include <vector>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\n"; \
            return 1; \
        } \
    } while (0)

static std::vector<uint32_t> ReadSpvFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        return {};
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    return buffer;
}

int main() {
    std::cout << "[Smoke Test] Starting vulkan_graphics_pipeline_smoke...\n";

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

    // 1. Create a dummy RenderPass for testing Pipeline creation
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkResult rpRes = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    TEST_CHECK(rpRes == VK_SUCCESS, "RenderPass creation failed");

    // 2. Load Shaders
    std::string shaderDir = NEO_SHADER_DIR;
    std::vector<uint32_t> vertSpv = ReadSpvFile(shaderDir + "/neo_triangle.vert.spv");
    std::vector<uint32_t> fragSpv = ReadSpvFile(shaderDir + "/neo_triangle.frag.spv");

    TEST_CHECK(!vertSpv.empty(), "Vertex shader SPV read failed");
    TEST_CHECK(!fragSpv.empty(), "Fragment shader SPV read failed");

    // 3. Test Shader Module standalone creation
    VkShaderModule testModule = NeoEngine::VulkanGraphicsPipeline::CreateShaderModule(device, vertSpv);
    TEST_CHECK(testModule != VK_NULL_HANDLE, "CreateShaderModule returned NULL");
    vkDestroyShaderModule(device, testModule, nullptr);

    // 4. Configure & Initialize Graphics Pipeline
    NeoEngine::VulkanPipelineConfig config{};
    config.renderPass = renderPass;
    config.subpass = 0;
    config.vertexSpv = vertSpv;
    config.fragmentSpv = fragSpv;
    config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    config.cullMode = VK_CULL_MODE_NONE;
    config.depthTestEnable = false;
    config.depthWriteEnable = false;

    NeoEngine::VulkanGraphicsPipeline pipeline;
    bool init = pipeline.Initialize(device, config);
    TEST_CHECK(init, "GraphicsPipeline initialization failed");
    TEST_CHECK(pipeline.IsValid(), "GraphicsPipeline IsValid check failed");
    TEST_CHECK(pipeline.GetPipeline() != VK_NULL_HANDLE, "GetPipeline returned NULL");
    TEST_CHECK(pipeline.GetPipelineLayout() != VK_NULL_HANDLE, "GetPipelineLayout returned NULL");

    // 5. Test Move Semantics
    NeoEngine::VulkanGraphicsPipeline movedPipeline = std::move(pipeline);
    TEST_CHECK(!pipeline.IsValid(), "Moved-from pipeline should be invalid");
    TEST_CHECK(movedPipeline.IsValid(), "Moved-to pipeline should be valid");

    // RAII Cleanup
    movedPipeline.Destroy();
    vkDestroyRenderPass(device, renderPass, nullptr);
    context.Reset();

    std::cout << "[Smoke Test] vulkan_graphics_pipeline_smoke passed successfully!\n";
    return 0;
}
