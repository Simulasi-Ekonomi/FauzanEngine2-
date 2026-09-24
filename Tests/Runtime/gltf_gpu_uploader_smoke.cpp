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

    std::vector<Vertex> vertices(3);
    vertices[0].position[0] = 0.0F; vertices[0].position[1] = 0.0F; vertices[0].position[2] = 0.0F;
    vertices[1].position[0] = 1.0F; vertices[1].position[1] = 0.0F; vertices[1].position[2] = 0.0F;
    vertices[2].position[0] = 0.0F; vertices[2].position[1] = 1.0F; vertices[2].position[2] = 0.0F;
    for (auto& vertex : vertices) {
        vertex.normal[0] = 0.0F; vertex.normal[1] = 0.0F; vertex.normal[2] = 1.0F;
        vertex.uv[0] = 0.0F; vertex.uv[1] = 0.0F;
        vertex.boneIndices[0] = 0U; vertex.boneIndices[1] = 1U;
        vertex.boneWeights[0] = 0.75F; vertex.boneWeights[1] = 0.25F;
    }
    const std::vector<std::uint32_t> indices{0U, 1U, 2U};

    if (!uploader.UploadMesh(vertices, indices)) return 4;
    if (!uploader.GetMeshBuffer().IsValid()) return 5;
    if (uploader.GetMeshBuffer().GetVertexCount() != 3U || uploader.GetMeshBuffer().GetIndexCount() != 3U) return 6;

    std::vector<MeshVertex3D> uploaded(3);
    if (!uploader.GetMeshBuffer().GetVertexBuffer().ReadData(uploaded.data(), sizeof(MeshVertex3D) * uploaded.size())) return 7;
    if (uploaded[1].boneIndices[0] != 0U || uploaded[1].boneIndices[1] != 1U) return 8;
    if (uploaded[1].boneWeights[0] != 0.75F || uploaded[1].boneWeights[1] != 0.25F) return 9;

    Vertex invalid = vertices[0];
    invalid.position[0] = 1.0e30F;
    if (uploader.UploadMesh({invalid, vertices[1], vertices[2]}, indices)) return 10;
    if (!uploader.GetMeshBuffer().IsValid() || uploader.GetMeshBuffer().GetVertexCount() != 3U) return 11;

    uploader.Destroy();
    if (uploader.IsValid() || uploader.GetMeshBuffer().IsValid()) return 12;
    context.Reset();
    std::puts("GLTF_GPU_UPLOADER_SMOKE_OK init=1 upload=1 buffer=1 skinning_payload=1 validation=1 destroy=1");
    return 0;
}
