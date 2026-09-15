#include "Runtime/ShaderLibrary.h"
#include "Runtime/VulkanContext.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::vector<uint32_t> ReadSpirV(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};

    const std::streamsize size = file.tellg();
    if (size <= 0 || (size % static_cast<std::streamsize>(sizeof(uint32_t))) != 0) return {};

    std::vector<uint32_t> data(static_cast<size_t>(size) / sizeof(uint32_t));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) return {};
    return data;
}

#define CHECK(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "[FAIL] " << message << "\n"; \
            return 1; \
        } \
    } while (false)

} // namespace

int main(int argc, char** argv) {
    std::cout << "[Smoke Test] ShaderLibrary\n";

    NeoEngine::VulkanContext context;
    if (!context.Initialize()) {
        std::cout << "[SKIP] Vulkan context unavailable\n";
        return 0;
    }

    NeoEngine::ShaderLibrary library;
    CHECK(library.Initialize(context.GetDevice()), "ShaderLibrary initialization failed");
    CHECK(library.IsValid(), "ShaderLibrary should be valid after initialization");

    CHECK(library.GetSampler(NeoEngine::TextureSampler::PointClamp) != VK_NULL_HANDLE,
          "PointClamp sampler missing");
    CHECK(library.GetSampler(NeoEngine::TextureSampler::LinearClamp) != VK_NULL_HANDLE,
          "LinearClamp sampler missing");
    CHECK(library.GetSampler(NeoEngine::TextureSampler::LinearRepeat) != VK_NULL_HANDLE,
          "LinearRepeat sampler missing");
    CHECK(library.GetSampler(NeoEngine::TextureSampler::LinearClampMipmap) != VK_NULL_HANDLE,
          "LinearClampMipmap sampler missing");

    const std::vector<uint32_t> invalidSpirV = {0x07230203U, 0x00010000U};
    CHECK(library.CompileShader("invalid", VK_SHADER_STAGE_VERTEX_BIT, invalidSpirV) == VK_NULL_HANDLE,
          "Truncated SPIR-V must be rejected");

    const std::vector<uint32_t> invalidInstruction = {
        0x07230203U, 0x00010000U, 0U, 1U, 0U, 0x00030001U};
    CHECK(library.CompileShader("invalid-instruction", VK_SHADER_STAGE_VERTEX_BIT, invalidInstruction) == VK_NULL_HANDLE,
          "Truncated SPIR-V instruction must be rejected");

    if (argc >= 3) {
        const std::vector<uint32_t> vertexSpirV = ReadSpirV(argv[1]);
        const std::vector<uint32_t> fragmentSpirV = ReadSpirV(argv[2]);
        CHECK(!vertexSpirV.empty(), "Vertex SPIR-V fixture could not be read");
        CHECK(!fragmentSpirV.empty(), "Fragment SPIR-V fixture could not be read");

        const VkShaderModule vertex = library.CompileShader(
            "pbr_vertex", VK_SHADER_STAGE_VERTEX_BIT, vertexSpirV, "default");
        const VkShaderModule vertexCached = library.CompileShader(
            "pbr_vertex", VK_SHADER_STAGE_VERTEX_BIT, vertexSpirV, "default");
        CHECK(vertex != VK_NULL_HANDLE, "Vertex shader module creation failed");
        CHECK(vertex == vertexCached, "Identical shader variant must use cache");
        CHECK(library.GetShaderModule("pbr_vertex", VK_SHADER_STAGE_VERTEX_BIT, "default") == vertex,
              "Cached vertex module lookup failed");

        const VkShaderModule fragment = library.CompileShader(
            "pbr_lighting", VK_SHADER_STAGE_FRAGMENT_BIT, fragmentSpirV, "normal-map");
        CHECK(fragment != VK_NULL_HANDLE, "Fragment shader module creation failed");
        CHECK(library.ShaderCount() == 2, "Expected two cached shader modules");

        CHECK(library.ReloadShader("pbr_lighting", VK_SHADER_STAGE_FRAGMENT_BIT,
                                   fragmentSpirV, "normal-map"),
              "Shader reload failed");
        CHECK(library.GetShaderModule("pbr_lighting", VK_SHADER_STAGE_FRAGMENT_BIT,
                                      "normal-map") != VK_NULL_HANDLE,
              "Reloaded shader module missing");
    } else {
        std::cout << "[INFO] No SPIR-V paths supplied; sampler/cache validation only\n";
    }

    library.Destroy();
    CHECK(!library.IsValid(), "ShaderLibrary should be invalid after Destroy");
    std::cout << "[PASS] ShaderLibrary validation completed\n";
    return 0;
}
