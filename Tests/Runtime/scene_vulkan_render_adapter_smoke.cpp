#include "Runtime/RenderCamera.h"
#include "Runtime/SceneMeshAdapter.h"
#include "Runtime/SceneRenderAdapter.h"
#include "Runtime/SceneWorld.h"
#include "Runtime/Vulkan3DRenderer.h"

#include <cassert>
#include <cstdio>

int main() {
    using namespace NeoEngine;

    SceneWorld world;
    SceneEntity entity{};
    assert(world.Create(entity));
    assert(world.SetTransform(entity, {0.0F, 0.0F, 3.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}));
    assert(world.UpdateTransforms());

    SceneMeshAdapter meshes;
    SceneMeshInstance mesh{};
    mesh.entity = entity;
    mesh.vertices = {
        {{-0.8F, -0.6F, 0.0F}, {0.0F, 0.0F, 1.0F}, 0.0F, 0.0F},
        {{0.8F, -0.6F, 0.0F}, {0.0F, 0.0F, 1.0F}, 1.0F, 0.0F},
        {{0.0F, 0.8F, 0.0F}, {0.0F, 0.0F, 1.0F}, 0.5F, 1.0F}
    };
    mesh.indices = {0, 1, 2};
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
