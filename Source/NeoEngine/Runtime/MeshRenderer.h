#pragma once

#include "RenderCamera.h"

#include <array>\n#include <cstdint>
#include <vector>

namespace NeoEngine {
class SoftwareRenderer;
struct CpuTextureResource;
enum class MeshRenderError : uint8_t { None, EmptyMesh, Capacity, InvalidIndex, InvalidTransform, InvalidLight, InvalidTexture, ProjectionFailed, RasterFailed };
struct MeshVertex {\n    RenderPoint3 position{};\n    RenderPoint3 normal{0.0F, 0.0F, 1.0F};\n    float u = 0.0F;\n    float v = 0.0F;\n    std::array<uint32_t, 4> boneIndices{};\n    std::array<float, 4> boneWeights{};\n};
struct MeshMaterial { uint32_t rgba = 0xFFFFFFFF; float ambient = 0.20F; float directional = 0.80F; const CpuTextureResource* texture = nullptr; bool cullBackFaces = false; };
struct MeshTransform { RenderPoint3 translation{}; float uniformScale = 1.0F; RenderPoint3 rotationRadians{}; };
struct DirectionalLight { RenderPoint3 directionToLight{0.0F, 0.0F, 1.0F}; float intensity = 1.0F; };
class MeshRenderer {
public:
    static constexpr uint16_t kMaxVertices = 2048;
    static constexpr uint16_t kMaxIndices = 6144;
    bool Draw(const std::vector<MeshVertex>& vertices, const std::vector<uint16_t>& indices, const MeshTransform& transform, const MeshMaterial& material, const DirectionalLight& light, RenderCamera& camera, SoftwareRenderer& renderer);
    [[nodiscard]] MeshRenderError LastError() const { return lastError_; }
private:
    MeshRenderError lastError_ = MeshRenderError::None;
};
} // namespace NeoEngine
