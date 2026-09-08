#pragma once

#include <array>
#include <vector>

struct BoundingSphere
{
    float x;
    float y;
    float z;
    float radius;
};

struct FrustumPlane
{
    float x;
    float y;
    float z;
    float d;
};

class GPUFrustumCulling
{
public:
    // Existing API preserved for compatibility.
    void AddObject(const BoundingSphere& sphere);
    void PerformCulling();
    const std::vector<int>& VisibleObjects() const;

    // R4: view-projection is a standard column-major 4x4 matrix, matching
    // the Vulkan GLSL mat4 representation used by CameraUBO.
    void SetViewProjectionMatrix(const float viewProjection[16]);
    void PerformCulling(const float viewProjection[16]);

    [[nodiscard]] bool HasFrustum() const { return hasFrustum_; }
    [[nodiscard]] const std::array<FrustumPlane, 6>& FrustumPlanes() const { return planes_; }

private:
    static FrustumPlane NormalizePlane(FrustumPlane plane);
    static bool SphereInsideFrustum(const BoundingSphere& sphere,
                                    const std::array<FrustumPlane, 6>& planes);

    std::vector<BoundingSphere> objects;
    std::vector<int> visible;
    std::array<FrustumPlane, 6> planes_{};
    bool hasFrustum_ = false;
};
