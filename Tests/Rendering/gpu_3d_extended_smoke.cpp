#include "Renderer/GPUDrivenRenderer.h"
#include "Renderer/GPUFrustumCulling.h"
#include "Renderer/Resources/BindlessResourceManager.h"
#include "Renderer/VirtualGeometry/VirtualGeometrySystem.h"
#include "Rendering/Rendering/Renderer/FrustumCulling.h"
#include "Rendering/Rendering/Renderer/HiZBuffer.h"
#include "Rendering/Rendering/Renderer/OcclusionBuffer.h"
#include "Rendering/Rendering/Renderer/OcclusionCulling.h"
#include <cstdio>
#include <vector>

int main() {
    HiZBuffer hiz(4, 4);
    hiz.Build(std::vector<float>(16, 0.25F));
    if (hiz.GetLevels() != 3 || hiz.Sample(0, 3, 3) != 0.25F || hiz.Sample(1, 1, 1) != 0.25F || hiz.Sample(9, 0, 0) != 1.0F) return 1;
    hiz.Build(std::vector<float>{1.0F});

    OcclusionBuffer depth(2, 2);
    depth.SetDepth(1, 1, 0.2F);
    if (depth.GetWidth() != 2 || depth.GetHeight() != 2 || depth.GetDepth(1, 1) != 0.2F) return 2;
    depth.Clear();
    if (depth.GetDepth(1, 1) != 1.0F) return 3;

    Frustum frustum{};
    for (auto& plane : frustum.planes) plane = {0.0F, 0.0F, 0.0F, 1.0F};
    FrustumCulling culling;
    if (!culling.IsVisible(frustum, {{-1, -1, -1}, {1, 1, 1}})) return 4;
    frustum.planes[0] = {1.0F, 0.0F, 0.0F, -10.0F};
    if (culling.IsVisible(frustum, {{-1, -1, -1}, {1, 1, 1}})) return 5;

    HiZBuffer occlusionHiz(2, 2);
    occlusionHiz.Build({0.5F, 0.5F, 0.5F, 0.5F});
    OcclusionCulling occlusion;
    if (!occlusion.IsOccluded({{0, 0, 0.75F}, {1, 1, 1.0F}}, occlusionHiz) || occlusion.IsOccluded({{0, 0, 0.25F}, {1, 1, 0.5F}}, occlusionHiz)) return 6;

    GPUFrustumCulling gpuCulling;
    gpuCulling.AddObject({0, 0, 0, 1});
    gpuCulling.AddObject({0, 0, 0, 0});
    gpuCulling.PerformCulling();
    if (gpuCulling.VisibleObjects().size() != 1 || gpuCulling.VisibleObjects()[0] != 0) return 7;

    VirtualGeometrySystem geometry;
    geometry.AddCluster({1, 64});
    geometry.AddCluster({2, 128});
    geometry.StreamVisible();
    if (geometry.Clusters().size() != 2 || geometry.Clusters()[1].triangleCount != 128) return 8;

    BindlessResourceManager bindless;
    const GPUResourceHandle first = bindless.RegisterResource();
    const GPUResourceHandle second = bindless.RegisterResource();
    if (first.id != 0 || second.id != 1 || bindless.ResourceCount() != 2) return 9;
    bindless.RemoveResource(first.id);
    if (bindless.ResourceCount() != 2) return 10;

    GPUDrivenRenderer driven;
    driven.Initialize(VK_NULL_HANDLE);
    if (driven.IsInitialized() || driven.TrySubmitDraw({3, 1, 0, 0, 0}) || driven.PendingDrawCount() != 0) return 11;
    driven.Initialize(reinterpret_cast<VkDevice>(static_cast<uintptr_t>(1)));
    if (!driven.IsInitialized() || !driven.TrySubmitDraw({3, 1, 0, 0, 0}) || driven.TrySubmitDraw({0, 1, 0, 0, 0}) || driven.PendingDrawCount() != 1) return 12;
    driven.Execute(VK_NULL_HANDLE);
    if (driven.PendingDrawCount() != 1) return 13;

    std::puts("GPU_3D_EXTENDED_SMOKE_OK hiz=1 occlusion=1 frustum=1 gpu_culling=1 virtual_geometry=1 bindless=1 indirect_guard=1");
    return 0;
}
