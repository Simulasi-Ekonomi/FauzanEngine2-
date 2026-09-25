#include "Asset/GLTF/GLTFGPUUploader.h"
#include "Asset/GLTF/GLTFLoader.h"
#include "Runtime/VulkanContext.h"

#include <cstdio>
#include <string>
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

    // Exercise the production import-to-GPU path with a bounded glTF 2.0 payload:
    // data-URI buffer -> accessor/skin data -> GLTFMesh -> Vulkan mesh buffers.
    const std::string gltfJson = R"json({"asset":{"version":"2.0"},"buffers":[{"byteLength":102,"uri":"data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAEAAIAAAAAAAQAAAAABAAAAAIA/AAAAAAAAAAAAAAAAAAAAPwAAAD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAA="}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":6},{"buffer":0,"byteOffset":42,"byteLength":12},{"buffer":0,"byteOffset":54,"byteLength":48}],"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},{"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"},{"bufferView":2,"componentType":5121,"count":3,"type":"VEC4"},{"bufferView":3,"componentType":5126,"count":3,"type":"VEC4"}],"meshes":[{"primitives":[{"attributes":{"POSITION":0,"JOINTS_0":2,"WEIGHTS_0":3},"indices":1}]}]})json";
    GLTFLoader loader;
    if (!loader.ParseMeshesChecked(gltfJson) || loader.GetMeshes().size() != 1U) return 12;
    const MeshData& imported = loader.GetMeshes().front().meshData;
    if (imported.vertices.size() != 3U || imported.indices.size() != 3U) return 13;
    if (!uploader.UploadMesh(imported.vertices, imported.indices)) return 14;
    if (!uploader.GetMeshBuffer().IsValid() ||
        uploader.GetMeshBuffer().GetVertexCount() != imported.vertices.size() ||
        uploader.GetMeshBuffer().GetIndexCount() != imported.indices.size()) return 15;

    uploaded.assign(imported.vertices.size(), MeshVertex3D{});
    if (!uploader.GetMeshBuffer().GetVertexBuffer().ReadData(uploaded.data(), sizeof(MeshVertex3D) * uploaded.size())) return 16;
    if (uploaded[1].boneIndices[0] != imported.vertices[1].boneIndices[0] ||
        uploaded[1].boneWeights[0] != imported.vertices[1].boneWeights[0]) return 17;

    uploader.Destroy();
    if (uploader.IsValid() || uploader.GetMeshBuffer().IsValid()) return 18;
    context.Reset();
    std::puts("GLTF_GPU_UPLOADER_SMOKE_OK init=1 direct_upload=1 gltf_import_to_vulkan=1 skinning_payload=1 validation=1 destroy=1");
    return 0;
}
