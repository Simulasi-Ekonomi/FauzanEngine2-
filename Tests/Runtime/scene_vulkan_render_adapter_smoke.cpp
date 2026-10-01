#include "Runtime/RenderCamera.h"
#include "Runtime/SceneMeshAdapter.h"
#include "Runtime/SceneRenderAdapter.h"
#include "Runtime/SceneWorld.h"
#include "Runtime/Vulkan3DRenderer.h"

#include <cstdlib>
#include <iostream>
#include <utility>

#define REQUIRE(...) do { if (!(__VA_ARGS__)) { std::cerr << "SCENE_VULKAN_REQUIRE_FAIL:" << __FILE__ << ":" << __LINE__ << " expr=" << #__VA_ARGS__ << std::endl; return 1; } } while (false)

int main() {
    using namespace NeoEngine;

    SceneWorld world;
    SceneEntity entity{};
    REQUIRE(world.Create(entity));
    REQUIRE(world.SetTransform(entity, {0.0F, 0.0F, 3.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}));
    REQUIRE(world.UpdateTransforms());

    SceneMeshAdapter meshes;
    SceneMeshInstance mesh{};
    mesh.entity = entity;
    mesh.vertices = {
        {{-0.8F, -0.6F, 0.0F}, {0.0F, 0.0F, 1.0F}, 0.0F, 0.0F},
        {{0.8F, -0.6F, 0.0F}, {0.0F, 0.0F, 1.0F}, 1.0F, 0.0F},
        {{0.0F, 0.8F, 0.0F}, {0.0F, 0.0F, 1.0F}, 0.5F, 1.0F}
    };
    mesh.indices = {0, 1, 2};
    REQUIRE(meshes.Add(std::move(mesh)));

    RenderCamera camera;
    RenderCameraConfig cameraConfig{};
    cameraConfig.mode = RenderCameraMode::Perspective;
    cameraConfig.position = {0.0F, 0.0F, 0.0F};
    cameraConfig.forward = {0.0F, 0.0F, 1.0F};
    cameraConfig.up = {0.0F, 1.0F, 0.0F};
    cameraConfig.aspect = 1.0F;
    cameraConfig.nearPlane = 0.1F;
    cameraConfig.farPlane = 100.0F;
    REQUIRE(camera.Initialize(cameraConfig));

    Vulkan3DRenderer renderer;
    if (!renderer.Initialize(256, 256, "NeoEngine Scene 3D Smoke")) return 2;

    SceneRenderAdapter adapter;
    if (!adapter.DrawVulkan3D(world, meshes, camera, renderer)) return 3;
    REQUIRE(renderer.LastFrameStats().indexCount == 3U);
    REQUIRE(renderer.LastFrameStats().vertexCount == 3U);
    return 0;
}
