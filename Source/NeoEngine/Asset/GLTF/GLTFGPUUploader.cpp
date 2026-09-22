#include "GLTFGPUUploader.h"

#include <cmath>
#include <limits>

namespace NeoEngine {

bool GLTFGPUUploader::Initialize(VkDevice device, VkPhysicalDevice physicalDevice) noexcept {
    Destroy();
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE) return false;
    device_ = device;
    physicalDevice_ = physicalDevice;
    return true;
}

void GLTFGPUUploader::Destroy() noexcept {
    meshBuffer_.Destroy();
    device_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
}

bool GLTFGPUUploader::UploadMesh(const std::vector<Vertex>& vertices,
                                 const std::vector<std::uint32_t>& indices) {
    if (!IsValid() || vertices.empty() || indices.empty() || indices.size() % 3U != 0U) return false;
    constexpr std::size_t kMaxVertices = 1U << 20U;
    constexpr std::size_t kMaxIndices = 3U * (1U << 20U);
    if (vertices.size() > kMaxVertices || indices.size() > kMaxIndices) return false;
    for (const Vertex& vertex : vertices) {
        for (float value : {vertex.position[0], vertex.position[1], vertex.position[2], vertex.normal[0], vertex.normal[1], vertex.normal[2], vertex.uv[0], vertex.uv[1]}) {
            if (!std::isfinite(value) || std::fabs(value) > 1.0e12F) return false;
        }
    }
    for (std::uint32_t index : indices) if (index >= vertices.size()) return false;
    return meshBuffer_.Build(device_, physicalDevice_, vertices, indices);
}

} // namespace NeoEngine
