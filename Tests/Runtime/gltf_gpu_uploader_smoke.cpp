#include "Asset/GLTF/GLTFGPUUploader.h"
#include "Runtime/VulkanContext.h"

#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    VulkanContext context;
    if (!context.Initialize()) {
        std::puts("GLTF_GPU_UPLOADER_SMOKE_SKIPPED_NO_VULKAN");
        return 0;
    }

    GLTFGPUUploader uploader;
    if (uploader.IsValid()) return 1;
    if (!uploader.Initialize(context.Device(), context.PhysicalDevice()) || !uploader.IsValid()) return 2;
    if (uploader.UploadMesh({}, {})) return 3;

    std::vector<GLTFGPUUploader::Vertex> vertices(3);
    vertices[0].position[0] = 0.0F; vertices[0].position[1] = 0.0F; vertices[0].position[2] = 0.0F;
    vertices[1].position[0] = 1.0F; vertices[1].position[1] = 0.0F; vertices[1].position[2] = 0.0F;
    vertices[2].position[0] = 0.0F; vertices[2].position[1] = 1.0F; vertices[2].position[2] = 0.0F;
    for (auto& vertex : vertices) {
        vertex.normal[0] = 0.0F; vertex.normal[1] = 0.0F; vertex.normal[2] = 1.0F;
        vertex.uv[0] = 0.0F; vertex.uv[1] = 0.0F;
    }
    const std::vector<std::uint32_t> indices{0U, 1U, 2U};

    if (!uploader.UploadMesh(vertices, indices)) return 4;
    if (!uploader.GetMeshBuffer().IsValid()) return 5;
    if (uploader.GetMeshBuffer().GetVertexCount() != 3U || uploader.GetMeshBuffer().GetIndexCount() != 3U) return 6;

    GLTFGPUUploader::Vertex invalid = vertices[0];
    invalid.position[0] = 1.0e30F;
    if (uploader.UploadMesh({invalid, vertices[1], vertices[2]}, indices)) return 7;
    if (!uploader.GetMeshBuffer().IsValid() || uploader.GetMeshBuffer().GetVertexCount() != 3U) return 8;

    uploader.Destroy();
    if (uploader.IsValid() || uploader.GetMeshBuffer().IsValid()) return 9;
    context.Reset();
    std::puts("GLTF_GPU_UPLOADER_SMOKE_OK init=1 upload=1 buffer=1 validation=1 destroy=1");
    return 0;
}
