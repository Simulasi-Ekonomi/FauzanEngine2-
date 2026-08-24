#include "VulkanShaderProbe.h"

#include "VulkanContext.h"

#include <fstream>
#include <vector>

namespace NeoEngine {
namespace {

std::vector<uint32_t> ReadSpirv(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file || file.tellg() <= 0 || static_cast<size_t>(file.tellg()) % sizeof(uint32_t) != 0) {
        return {};
    }
    const size_t byteCount = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> words(byteCount / sizeof(uint32_t));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(words.data()), static_cast<std::streamsize>(byteCount))) {
        return {};
    }
    return words;
}

} // namespace

bool VulkanShaderProbe::CreateTriangleModules() {
    VulkanContext context;
    if (!context.Initialize()) {
        return false;
    }
    const std::vector<uint32_t> vertexCode = ReadSpirv(NEO_SHADER_DIR "/neo_triangle.vert.spv");
    const std::vector<uint32_t> fragmentCode = ReadSpirv(NEO_SHADER_DIR "/neo_triangle.frag.spv");
    if (vertexCode.empty() || fragmentCode.empty()) {
        return false;
    }

    VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    moduleInfo.codeSize = vertexCode.size() * sizeof(uint32_t);
    moduleInfo.pCode = vertexCode.data();
    VkShaderModule vertexModule = VK_NULL_HANDLE;
    VkShaderModule fragmentModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(context.Device(), &moduleInfo, nullptr, &vertexModule) != VK_SUCCESS) {
        return false;
    }
    moduleInfo.codeSize = fragmentCode.size() * sizeof(uint32_t);
    moduleInfo.pCode = fragmentCode.data();
    const bool created = vkCreateShaderModule(context.Device(), &moduleInfo, nullptr, &fragmentModule) == VK_SUCCESS;
    if (fragmentModule != VK_NULL_HANDLE) vkDestroyShaderModule(context.Device(), fragmentModule, nullptr);
    vkDestroyShaderModule(context.Device(), vertexModule, nullptr);
    return created;
}

} // namespace NeoEngine
