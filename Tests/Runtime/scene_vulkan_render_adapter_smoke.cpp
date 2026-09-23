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
    assert(meshes.Add(std::move(mesh)));

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
    if (!renderer.Initialize(256, 256, "NeoEngine Scene 3D Smoke")) {\n        std::fprintf(stderr, "SCENE_VULKAN_ADAPTER_FAIL init error=%u\\n", static_cast<unsigned>(renderer.LastError()));\n        return 2;\n    }

    SceneRenderAdapter adapter;
    if (!adapter.DrawVulkan3D(world, meshes, camera, renderer)) return 3;
    if (renderer.LastFrameStats().indexCount != 3U) return 4;
    if (renderer.LastFrameStats().vertexCount != 3U) return 5;
    return 0;
}
