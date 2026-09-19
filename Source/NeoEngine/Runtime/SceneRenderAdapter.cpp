#include "Runtime/SceneRenderAdapter.h"

#include "Runtime/SoftwareRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace NeoEngine {
namespace {
struct Mat4 { std::array<float, 16> v{}; };

Mat4 Multiply(const Mat4& a, const Mat4& b) {
    Mat4 out{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float value = 0.0F;
            for (int k = 0; k < 4; ++k) value += a.v[k * 4 + row] * b.v[column * 4 + k];
            out.v[column * 4 + row] = value;
        }
    }
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
    Mat4 view{};
    view.v = {
        right.x, up.x, forward.x, 0.0F,
        right.y, up.y, forward.y, 0.0F,
        right.z, up.z, forward.z, 0.0F,
        -(right.x * config.position.x + right.y * config.position.y + right.z * config.position.z),
        -(up.x * config.position.x + up.y * config.position.y + up.z * config.position.z),
        -(forward.x * config.position.x + forward.y * config.position.y + forward.z * config.position.z),
        1.0F
    };
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
        projection.v = {
            1.0F / halfWidth, 0.0F, 0.0F, 0.0F,
            0.0F, 1.0F / halfHeight, 0.0F, 0.0F,
            0.0F, 0.0F, 1.0F / (farPlane - nearPlane), 0.0F,
            0.0F, 0.0F, -nearPlane / (farPlane - nearPlane), 1.0F
        };
        return projection;
    }

    constexpr float kPi = 3.14159265358979323846F;
    const float aspect = std::max(0.001F, config.aspect);
    const float nearPlane = std::max(0.001F, config.nearPlane);
    const float farPlane = std::max(nearPlane + 0.001F, config.farPlane);
    const float halfFov = std::clamp(config.verticalFovDegrees, 1.0F, 179.0F) * (kPi / 360.0F);
    const float focal = 1.0F / std::tan(halfFov);
    projection.v = {
        focal / aspect, 0.0F, 0.0F, 0.0F,
        0.0F, focal, 0.0F, 0.0F,
        0.0F, 0.0F, farPlane / (farPlane - nearPlane), 1.0F,
        0.0F, 0.0F, -(nearPlane * farPlane) / (farPlane - nearPlane), 0.0F
    };
    return projection;
}

Mat4 MakeModel(const Transform3& transform) {
    const float cx = std::cos(transform.rx), sx = std::sin(transform.rx);
    const float cy = std::cos(transform.ry), sy = std::sin(transform.ry);
    const float cz = std::cos(transform.rz), sz = std::sin(transform.rz);
    Mat4 model{};
    model.v = {
        (cz * cy) * transform.sx, (sz * cy) * transform.sx, (-sy) * transform.sx, 0.0F,
        (cz * sy * sx - sz * cx) * transform.sy, (sz * sy * sx + cz * cx) * transform.sy, (cy * sx) * transform.sy, 0.0F,
        (cz * sy * cx + sz * sx) * transform.sz, (sz * sy * cx - cz * sx) * transform.sz, (cy * cx) * transform.sz, 0.0F,
        transform.x, transform.y, transform.z, 1.0F
    };
    return model;
}

bool ValidateECSAssetIdentity(const SceneMeshInstance& instance, const ArchetypeManager& ecs,
                               const SceneECSBridge& sceneECS) {
    const EntityID ecsId = sceneECS.ECSId(instance.entity);
    if (!ecs.HasEntity(ecsId) || (ecs.GetComponentMask(ecsId) & COMP_MESH) == 0U) return false;
    uint64_t meshHash = 0U;
    uint64_t materialHash = 0U;
    if (!ecs.TryGetMeshAssetIdentity(ecsId, meshHash, materialHash)) return false;
    if (instance.sourceHash == 0U || meshHash != instance.sourceHash) return false;
    if (!instance.sourceMaterialAssetId.empty() &&
        (instance.sourceMaterialHash == 0U || materialHash != instance.sourceMaterialHash)) return false;
    return true;
}

bool MakeECSModel(const SceneMeshInstance& instance, const ArchetypeManager& ecs,
                  const SceneECSBridge& sceneECS, Mat4& model) {
    const EntityID ecsId = sceneECS.ECSId(instance.entity);
    if (!ecs.HasEntity(ecsId)) return false;

    float x = 0.0F, y = 0.0F, z = 0.0F;
    float rx = 0.0F, ry = 0.0F, rz = 0.0F;
    float sx = 1.0F, sy = 1.0F, sz = 1.0F;
    if (!ecs.TryGetPosition(ecsId, x, y, z) || !ecs.TryGetRotation(ecsId, rx, ry, rz) || !ecs.TryGetScale(ecsId, sx, sy, sz)) return false;

    Transform3 transform{};
    transform.x = x; transform.y = y; transform.z = z;
    transform.rx = rx; transform.ry = ry; transform.rz = rz;
    transform.sx = sx; transform.sy = sy; transform.sz = sz;
    model = MakeModel(transform);
    return true;
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
                                      Vulkan3DRenderer& renderer, float clearR, float clearG, float clearB, float clearA,
                                      const ArchetypeManager* ecs, const SceneECSBridge* sceneECS) {
    if (!renderer.Ready()) { lastError_ = SceneRenderAdapterError::VulkanFrameFailed; return false; }
    const RenderCameraConfig& config = camera.Config();
    if (config.aspect <= 0.0F || !std::isfinite(config.aspect) || config.nearPlane <= 0.0F ||
        config.farPlane <= config.nearPlane) {
        lastError_ = SceneRenderAdapterError::VulkanFrameFailed;
        return false;
    }
    if ((ecs == nullptr) != (sceneECS == nullptr)) {
        lastError_ = SceneRenderAdapterError::VulkanFrameFailed;
        return false;
    }

    const Mat4 viewProjection = Multiply(MakeProjection(config), MakeView(config, camera.Right(), camera.Up()));
    if (!renderer.BeginFrame(clearR, clearG, clearB, clearA)) {
        lastError_ = SceneRenderAdapterError::VulkanFrameFailed;
        return false;
    }

    struct Batch { size_t first = 0U; std::vector<size_t> instances; };
    std::vector<Batch> batches;
    batches.reserve(meshes.Instances().size());

    for (size_t i = 0U; i < meshes.Instances().size(); ++i) {
        const SceneMeshInstance& instance = meshes.Instances()[i];
        if (instance.vertices.empty() || instance.indices.empty() || instance.indices.size() % 3U != 0U) {
            lastError_ = SceneRenderAdapterError::VulkanMeshDrawFailed;
            renderer.EndFrame();
            return false;
        }

        Mat4 model{};
        if (ecs != nullptr) {
            if (!ValidateECSAssetIdentity(instance, *ecs, *sceneECS) || !MakeECSModel(instance, *ecs, *sceneECS, model)) {
                lastError_ = SceneRenderAdapterError::VulkanMeshDrawFailed;
                renderer.EndFrame();
                return false;
            }
        } else {
            const Transform3* transform = world.GetTransform(instance.entity);
            if (!transform) continue;
            model = MakeModel(*transform);
        }

        size_t batchIndex = batches.size();
        if (!instance.sourceAssetId.empty() && instance.sourceHash != 0U) {
            for (size_t b = 0U; b < batches.size(); ++b) {
                const SceneMeshInstance& first = meshes.Instances()[batches[b].first];
                if (first.sourceAssetId == instance.sourceAssetId &&
                    first.sourceHash == instance.sourceHash &&
                    first.sourceMaterialAssetId == instance.sourceMaterialAssetId &&
                    first.sourceMaterialHash == instance.sourceMaterialHash &&
                    first.vertices.size() == instance.vertices.size() &&
                    first.indices.size() == instance.indices.size()) {
                    batchIndex = b;
                    break;
                }
            }
        }
        if (batchIndex == batches.size()) batches.push_back({i, {}});
        batches[batchIndex].instances.push_back(i);
    }

    for (const Batch& batch : batches) {
        const SceneMeshInstance& first = meshes.Instances()[batch.first];
        std::vector<Vulkan3DVertex> vertices;
        vertices.reserve(first.vertices.size());
        for (const MeshVertex& vertex : first.vertices) {
            vertices.push_back(Vulkan3DVertex{
                vertex.position.x, vertex.position.y, vertex.position.z,
                vertex.normal.x, vertex.normal.y, vertex.normal.z,
                vertex.u, vertex.v});
        }

        std::vector<uint32_t> indices;
        indices.reserve(first.indices.size());
        for (uint16_t index : first.indices) {
            if (index >= first.vertices.size()) {
                lastError_ = SceneRenderAdapterError::VulkanMeshDrawFailed;
                renderer.EndFrame();
                return false;
            }
            indices.push_back(static_cast<uint32_t>(index));
        }

        std::vector<float> transforms;
        transforms.reserve(batch.instances.size() * 16U);
        for (const size_t instanceIndex : batch.instances) {
            Mat4 model{};
            if (ecs != nullptr) {
                if (!ValidateECSAssetIdentity(meshes.Instances()[instanceIndex], *ecs, *sceneECS) ||
                    !MakeECSModel(meshes.Instances()[instanceIndex], *ecs, *sceneECS, model)) {
                    lastError_ = SceneRenderAdapterError::VulkanMeshDrawFailed;
                    renderer.EndFrame();
                    return false;
                }
            } else {
                const Transform3* transform = world.GetTransform(meshes.Instances()[instanceIndex].entity);
                if (!transform) continue;
                model = MakeModel(*transform);
            }
            transforms.insert(transforms.end(), model.v.begin(), model.v.end());
        }

        if (transforms.empty() ||
            !renderer.DrawIndexedInstancedWithViewProjection(vertices, indices, transforms, viewProjection.v.data())) {
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