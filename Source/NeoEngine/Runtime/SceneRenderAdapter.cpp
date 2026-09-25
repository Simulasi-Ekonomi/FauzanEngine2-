#include "Runtime/SceneRenderAdapter.h"

#include "Runtime/SoftwareRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace NeoEngine {
namespace {

Mat4 Multiply(const Mat4& a, const Mat4& b) {
    Mat4 out{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float value = 0.0F;
            for (int k = 0; k < 4; ++k) value += a.m[k * 4 + row] * b.m[column * 4 + k];
            out.m[column * 4 + row] = value;
        }
    }
    return out;
}

Mat4 MakeMat4(const std::array<float, 16>& values) {
    Mat4 out{};
    std::copy(values.begin(), values.end(), out.m);
    return out;
}

RenderPoint3 Normalize(RenderPoint3 value) {
    const float length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    if (!std::isfinite(length) || length <= 1.0e-6F) return {0.0F, 0.0F, 1.0F};
    return {value.x / length, value.y / length, value.z / length};
}

Mat4 MakeView(const RenderCameraConfig& config, RenderPoint3 right, RenderPoint3 up) {
    const RenderPoint3 forward = Normalize(config.forward);
    right = Normalize(right);
    up = Normalize(up);
    const Mat4 view = MakeMat4({
        right.x, up.x, forward.x, 0.0F,
        right.y, up.y, forward.y, 0.0F,
        right.z, up.z, forward.z, 0.0F,
        -(right.x * config.position.x + right.y * config.position.y + right.z * config.position.z),
        -(up.x * config.position.x + up.y * config.position.y + up.z * config.position.z),
        -(forward.x * config.position.x + forward.y * config.position.y + forward.z * config.position.z),
        1.0F
    });
    return view;
}

Mat4 MakeProjection(const RenderCameraConfig& config) {
    Mat4 projection{};
    if (config.mode == RenderCameraMode::Orthographic) {
        const float halfHeight = std::max(0.001F, config.orthographicHalfHeight);
        const float aspect = std::max(0.001F, config.aspect);
        const float halfWidth = halfHeight * aspect;
        const float nearPlane = std::max(0.001F, config.nearPlane);
        const float farPlane = std::max(nearPlane + 0.001F, config.farPlane);
        projection = MakeMat4({
            1.0F / halfWidth, 0.0F, 0.0F, 0.0F,
            0.0F, 1.0F / halfHeight, 0.0F, 0.0F,
            0.0F, 0.0F, 1.0F / (farPlane - nearPlane), 0.0F,
            0.0F, 0.0F, -nearPlane / (farPlane - nearPlane), 1.0F
        });
        return projection;
    }

    constexpr float kPi = 3.14159265358979323846F;
    const float aspect = std::max(0.001F, config.aspect);
    const float nearPlane = std::max(0.001F, config.nearPlane);
    const float farPlane = std::max(nearPlane + 0.001F, config.farPlane);
    const float halfFov = std::clamp(config.verticalFovDegrees, 1.0F, 179.0F) * (kPi / 360.0F);
    const float focal = 1.0F / std::tan(halfFov);
    projection = MakeMat4({
        focal / aspect, 0.0F, 0.0F, 0.0F,
        0.0F, focal, 0.0F, 0.0F,
        0.0F, 0.0F, farPlane / (farPlane - nearPlane), 1.0F,
        0.0F, 0.0F, -(nearPlane * farPlane) / (farPlane - nearPlane), 0.0F
    });
    return projection;
}

Mat4 MakeModel(const Transform3& transform) {
    const float cx = std::cos(transform.rx), sx = std::sin(transform.rx);
    const float cy = std::cos(transform.ry), sy = std::sin(transform.ry);
    const float cz = std::cos(transform.rz), sz = std::sin(transform.rz);
    const Mat4 model = MakeMat4({
        (cz * cy) * transform.sx, (sz * cy) * transform.sx, (-sy) * transform.sx, 0.0F,
        (cz * sy * sx - sz * cx) * transform.sy, (sz * sy * sx + cz * cx) * transform.sy, (cy * sx) * transform.sy, 0.0F,
        (cz * sy * cx + sz * sx) * transform.sz, (sz * sy * cx - cz * sx) * transform.sz, (cy * cx) * transform.sz, 0.0F,
        transform.x, transform.y, transform.z, 1.0F
    });
    return model;
}

RenderPoint3 TransformPoint(const Mat4& matrix, RenderPoint3 point) {
    return {
        matrix.m[0] * point.x + matrix.m[4] * point.y + matrix.m[8] * point.z + matrix.m[12],
        matrix.m[1] * point.x + matrix.m[5] * point.y + matrix.m[9] * point.z + matrix.m[13],
        matrix.m[2] * point.x + matrix.m[6] * point.y + matrix.m[10] * point.z + matrix.m[14]
    };
}

RenderPoint3 TransformDirection(const Mat4& matrix, RenderPoint3 value) {
    return Normalize({
        matrix.m[0] * value.x + matrix.m[4] * value.y + matrix.m[8] * value.z,
        matrix.m[1] * value.x + matrix.m[5] * value.y + matrix.m[9] * value.z,
        matrix.m[2] * value.x + matrix.m[6] * value.y + matrix.m[10] * value.z
    });
}

} // namespace

bool SceneRenderAdapter::Draw(const SceneWorld& world, SceneMeshAdapter& meshes, const SceneSpriteAdapter& sprites, RenderCamera& camera, SoftwareRenderer& renderer, const DirectionalLight& light) {
    SoftwareRenderer candidate = renderer;
    if (!meshes.Draw(world, camera, candidate, light)) { lastError_ = SceneRenderAdapterError::MeshDrawFailed; return false; }
    SpriteBatch batch;
    if (!sprites.Queue(world, batch)) { lastError_ = SceneRenderAdapterError::SpriteQueueFailed; return false; }
    if (!batch.Flush(candidate, camera)) { lastError_ = SceneRenderAdapterError::SpriteFlushFailed; return false; }
    renderer = std::move(candidate); lastError_ = SceneRenderAdapterError::None; return true;
}

bool SceneRenderAdapter::DrawVulkan3D(const SceneWorld& world, const SceneMeshAdapter& meshes, RenderCamera& camera,
                                      Vulkan3DRenderer& renderer, float clearR, float clearG, float clearB, float clearA, const std::vector<Mat4>* skeletalPalette) {
    if (!renderer.Ready()) { lastError_ = SceneRenderAdapterError::VulkanFrameFailed; return false; }
    if (!std::isfinite(clearR) || !std::isfinite(clearG) || !std::isfinite(clearB) || !std::isfinite(clearA) ||
        clearR < 0.0F || clearR > 1.0F || clearG < 0.0F || clearG > 1.0F || clearB < 0.0F || clearB > 1.0F || clearA < 0.0F || clearA > 1.0F) { lastError_ = SceneRenderAdapterError::VulkanFrameFailed; return false; }
    const RenderCameraConfig& config = camera.Config();
    if (config.aspect <= 0.0F || !std::isfinite(config.aspect) || config.nearPlane <= 0.0F || config.farPlane <= config.nearPlane) {
        lastError_ = SceneRenderAdapterError::VulkanFrameFailed;
        return false;
    }

    const Mat4 viewProjection = Multiply(MakeProjection(config), MakeView(config, camera.Right(), camera.Up()));
    if (!renderer.BeginFrame(clearR, clearG, clearB, clearA)) {
        lastError_ = SceneRenderAdapterError::VulkanFrameFailed;
        return false;
    }

    for (const SceneMeshInstance& instance : meshes.Instances()) {
        const Transform3* transform = world.GetTransform(instance.entity);
        if (!transform) continue;
        if (!std::isfinite(transform->x) || !std::isfinite(transform->y) || !std::isfinite(transform->z) ||
            !std::isfinite(transform->rx) || !std::isfinite(transform->ry) || !std::isfinite(transform->rz) ||
            !std::isfinite(transform->sx) || !std::isfinite(transform->sy) || !std::isfinite(transform->sz)) { lastError_ = SceneRenderAdapterError::VulkanMeshDrawFailed; renderer.EndFrame(); return false; }
        if (instance.vertices.empty() || instance.indices.empty() || instance.indices.size() % 3U != 0U) {
            lastError_ = SceneRenderAdapterError::VulkanMeshDrawFailed;
            renderer.EndFrame();
            return false;
        }

        const Mat4 model = MakeModel(*transform);
        std::vector<Vulkan3DVertex> vertices;
        vertices.reserve(instance.vertices.size());
        for (const MeshVertex& vertex : instance.vertices) {
            const RenderPoint3 position = vertex.position;
            const RenderPoint3 normal = Normalize(vertex.normal);
            Vulkan3DVertex gpuVertex{position.x, position.y, position.z, normal.x, normal.y, normal.z, vertex.u, vertex.v};
            gpuVertex.boneIndices = vertex.boneIndices;
            gpuVertex.boneWeights = vertex.boneWeights;
            vertices.push_back(gpuVertex);
        }

        std::vector<uint32_t> indices;
        indices.reserve(instance.indices.size());
        for (uint16_t index : instance.indices) {
            if (index >= instance.vertices.size()) {
                lastError_ = SceneRenderAdapterError::VulkanMeshDrawFailed;
                renderer.EndFrame();
                return false;
            }
            indices.push_back(static_cast<uint32_t>(index));
        }

        if (!instance.skeletalPalette.empty()) {
            if (!renderer.UploadSkinningPalette(instance.skeletalPalette)) {
                lastError_ = SceneRenderAdapterError::VulkanFrameFailed;
                renderer.EndFrame();
                return false;
            }
        } else if (skeletalPalette != nullptr && !skeletalPalette->empty()) {
            if (!renderer.UploadSkinningPalette(*skeletalPalette)) {
                lastError_ = SceneRenderAdapterError::VulkanFrameFailed;
                renderer.EndFrame();
                return false;
            }
        } else if (!renderer.UseDefaultSkinningPalette()) {
            lastError_ = SceneRenderAdapterError::VulkanFrameFailed;
            renderer.EndFrame();
            return false;
        }
        if (!renderer.DrawIndexedSkinned(vertices, indices, Multiply(viewProjection, model).m, model.m)) {
            lastError_ = SceneRenderAdapterError::VulkanMeshDrawFailed;
            renderer.EndFrame();
            return false;
        }
    }

    if (!renderer.EndFrame()) {
        lastError_ = SceneRenderAdapterError::VulkanFrameFailed;
        return false;
    }
    lastError_ = SceneRenderAdapterError::None;
    return true;
}

} // namespace NeoEngine
