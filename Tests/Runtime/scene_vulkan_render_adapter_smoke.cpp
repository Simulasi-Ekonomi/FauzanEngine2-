#include "Asset/GLTF/GLTFLoader.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SceneMeshAdapter.h"
#include "Runtime/SceneRenderAdapter.h"
#include "Runtime/SceneWorld.h"
#include "Runtime/Vulkan3DRenderer.h"

#include <cassert>
#include <cstdio>
#include <limits>
#include <utility>
#include <vector>

int main() {
    using namespace NeoEngine;

    SceneWorld world;
    SceneEntity entity{};
    assert(world.Create(entity));
    assert(world.SetTransform(entity, {0.0F, 0.0F, 3.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}));
    assert(world.UpdateTransforms());

    const std::string gltfJson = R"json({"asset":{"version":"2.0"},"buffers":[{"byteLength":102,"uri":"data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAABAAIAAAAAAAEAAAAAAQAAAACAPwAAAAAAAAAAAAAAAAAAAD8AAAA/AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAA"}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":6},{"buffer":0,"byteOffset":42,"byteLength":12},{"buffer":0,"byteOffset":54,"byteLength":48}],"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},{"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"},{"bufferView":2,"componentType":5121,"count":3,"type":"VEC4"},{"bufferView":3,"componentType":5126,"count":3,"type":"VEC4"}],"meshes":[{"primitives":[{"attributes":{"POSITION":0,"JOINTS_0":2,"WEIGHTS_0":3},"indices":1}]}]})json";
    GLTFLoader loader;
    if (!loader.ParseMeshesChecked(gltfJson) || loader.GetMeshes().size() != 1U) return 1;
    const MeshData& imported = loader.GetMeshes().front().meshData;
    if (imported.vertices.size() != 3U || imported.indices.size() != 3U) return 9;

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
    assert(redMeshes.Add(mesh));
    mesh.material.rgba = 0xFF0000FFU;
    SceneMeshAdapter blueMeshes;
    assert(blueMeshes.Add(std::move(mesh)));

    RenderCamera camera;
    RenderCameraConfig cameraConfig{};
    cameraConfig.mode = RenderCameraMode::Perspective;
    cameraConfig.position = {0.0F, 0.0F, 0.0F};
    cameraConfig.forward = {0.0F, 0.0F, 1.0F};
    cameraConfig.up = {0.0F, 1.0F, 0.0F};
    cameraConfig.aspect = 1.0F;
    cameraConfig.nearPlane = 0.1F;
    cameraConfig.farPlane = 100.0F;
    assert(camera.Initialize(cameraConfig));

    Vulkan3DRenderer renderer;
    if (!renderer.Initialize(256, 256, "NeoEngine Scene 3D Smoke")) {
        std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL init error=%u\n",
                     static_cast<unsigned>(renderer.LastError()));
        return 2;
    }

    SceneRenderAdapter adapter;
    if (!adapter.DrawVulkan3D(world, redMeshes, camera, renderer)) return 3;
    if (renderer.LastFrameStats().indexCount != 3U) return 4;
    if (renderer.LastFrameStats().vertexCount != 3U) return 5;
    std::vector<uint8_t> redFrame;
    if (!renderer.ReadbackLastFrame(redFrame) || redFrame.empty()) return 6;
    if (!adapter.DrawVulkan3D(world, blueMeshes, camera, renderer)) return 7;
    std::vector<uint8_t> blueFrame;
    if (!renderer.ReadbackLastFrame(blueFrame) || blueFrame.size() != redFrame.size() || blueFrame == redFrame) return 8;
    std::puts("SCENE_VULKAN_ADAPTER_MATERIAL_COLOR_OK distinct_gpu_colors=1");
    return 0;
}
