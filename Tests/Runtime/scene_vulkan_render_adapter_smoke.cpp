#include "Asset/GLTF/GLTFLoader.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SceneMeshAdapter.h"
#include "Runtime/SceneRenderAdapter.h"
#include "Runtime/SceneWorld.h"
#include "Runtime/Vulkan3DRenderer.h"

#include <cstdio>
#include <limits>
#include <utility>
#include <vector>

int main() {
    using namespace NeoEngine;

    SceneWorld world;
    SceneEntity entity{};
    if (!world.Create(entity)) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=world_create\\n"); return 1; }
    if (!world.SetTransform(entity, {0.0F, 0.0F, 3.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F})) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=set_transform\\n"); return 2; }
    if (!world.UpdateTransforms()) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=update_transforms\\n"); return 3; }

    const std::string gltfJson = R"json({"asset":{"version":"2.0"},"buffers":[{"byteLength":102,"uri":"data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAABAAIAAAAAAAEAAAAAAQAAAACAPwAAAAAAAAAAAAAAAAAAAD8AAAA/AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAA"}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":6},{"buffer":0,"byteOffset":42,"byteLength":12},{"buffer":0,"byteOffset":54,"byteLength":48}],"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},{"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"},{"bufferView":2,"componentType":5121,"count":3,"type":"VEC4"},{"bufferView":3,"componentType":5126,"count":3,"type":"VEC4"}],"meshes":[{"primitives":[{"attributes":{"POSITION":0,"JOINTS_0":2,"WEIGHTS_0":3},"indices":1}]}]})json";
    GLTFLoader loader;
    if (!loader.ParseMeshesChecked(gltfJson)) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=gltf_parse\\n"); return 4; }
    if (loader.GetMeshes().size() != 1U) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=gltf_mesh_count count=%zu\\n", loader.GetMeshes().size()); return 5; }
    const MeshData& imported = loader.GetMeshes().front().meshData;
    if (imported.vertices.size() != 3U || imported.indices.size() != 3U) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=gltf_geometry vertices=%zu indices=%zu\\n", imported.vertices.size(), imported.indices.size()); return 6; }

    SceneMeshInstance mesh{};
    mesh.entity = entity;
    mesh.vertices.reserve(imported.vertices.size());
    for (const Vertex& source : imported.vertices) {
        MeshVertex vertex{};
        vertex.position = {source.position[0], source.position[1], source.position[2]};
        vertex.normal = {source.normal[0], source.normal[1], source.normal[2]};
        vertex.u = source.uv[0];
        vertex.v = source.uv[1];
        for (size_t i = 0U; i < 4U; ++i) {
            vertex.boneIndices[i] = source.boneIndices[i];
            vertex.boneWeights[i] = source.boneWeights[i];
        }
        mesh.vertices.push_back(vertex);
    }
    mesh.indices.reserve(imported.indices.size());
    for (unsigned int index : imported.indices) {
        if (index > std::numeric_limits<uint16_t>::max()) return 10;
        mesh.indices.push_back(static_cast<uint16_t>(index));
    }
    mesh.material.rgba = 0xFFFF0000U;
    SceneMeshAdapter redMeshes;
    if (!redMeshes.Add(mesh)) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=red_mesh_add\\n"); return 7; }
    mesh.material.rgba = 0xFF0000FFU;
    SceneMeshAdapter blueMeshes;
    if (!blueMeshes.Add(std::move(mesh))) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=blue_mesh_add\\n"); return 8; }

    RenderCamera camera;
    RenderCameraConfig cameraConfig{};
    cameraConfig.mode = RenderCameraMode::Perspective;
    cameraConfig.position = {0.0F, 0.0F, 0.0F};
    cameraConfig.forward = {0.0F, 0.0F, 1.0F};
    cameraConfig.up = {0.0F, 1.0F, 0.0F};
    cameraConfig.aspect = 1.0F;
    cameraConfig.nearPlane = 0.1F;
    cameraConfig.farPlane = 100.0F;
    if (!camera.Initialize(cameraConfig)) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=camera_init\\n"); return 9; }

    Vulkan3DRenderer renderer;
    if (!renderer.Initialize(256, 256, "NeoEngine Scene 3D Smoke")) {
        std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL init error=%u\n",
                     static_cast<unsigned>(renderer.LastError()));
        return 10;
    }

    SceneRenderAdapter adapter;
    if (!adapter.DrawVulkan3D(world, redMeshes, camera, renderer)) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=red_draw error=%u\\n", static_cast<unsigned>(adapter.LastError())); return 11; }
    if (renderer.LastFrameStats().indexCount != 3U) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=red_stats_indices value=%u\\n", renderer.LastFrameStats().indexCount); return 12; }
    if (renderer.LastFrameStats().vertexCount != 3U) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=red_stats_vertices value=%u\\n", renderer.LastFrameStats().vertexCount); return 13; }
    std::vector<uint8_t> redFrame;
    if (!renderer.ReadbackLastFrame(redFrame) || redFrame.empty()) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=red_readback error=%u size=%zu\\n", static_cast<unsigned>(renderer.LastError()), redFrame.size()); return 14; }
    if (!adapter.DrawVulkan3D(world, blueMeshes, camera, renderer)) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=blue_draw error=%u\\n", static_cast<unsigned>(adapter.LastError())); return 15; }
    std::vector<uint8_t> blueFrame;
    if (!renderer.ReadbackLastFrame(blueFrame) || blueFrame.size() != redFrame.size()) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=blue_readback error=%u red=%zu blue=%zu\\n", static_cast<unsigned>(renderer.LastError()), redFrame.size(), blueFrame.size()); return 16; }
    if (blueFrame == redFrame) { std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL stage=material_color_not_distinct\\n"); return 17; }
    std::puts("SCENE_VULKAN_ADAPTER_MATERIAL_COLOR_OK distinct_gpu_colors=1");
    return 0;
}
