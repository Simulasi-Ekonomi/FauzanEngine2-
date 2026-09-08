#include "GPUFrustumCulling.h"

#include <cmath>
#include <cstring>

void GPUFrustumCulling::AddObject(const BoundingSphere& sphere)
{
    objects.push_back(sphere);
}

void GPUFrustumCulling::SetViewProjectionMatrix(const float viewProjection[16])
{
    // Column-major matrix: each plane is extracted from row 3 +/- row 0/1/2.
    // The six planes are left, right, bottom, top, near and far.
    const float* m = viewProjection;

    planes_[0] = {m[3] + m[0],  m[7] + m[4],  m[11] + m[8],  m[15] + m[12]};
    planes_[1] = {m[3] - m[0],  m[7] - m[4],  m[11] - m[8],  m[15] - m[12]};
    planes_[2] = {m[3] + m[1],  m[7] + m[5],  m[11] + m[9],  m[15] + m[13]};
    planes_[3] = {m[3] - m[1],  m[7] - m[5],  m[11] - m[9],  m[15] - m[13]};
    planes_[4] = {m[3] + m[2],  m[7] + m[6],  m[11] + m[10], m[15] + m[14]};
    planes_[5] = {m[3] - m[2],  m[7] - m[6],  m[11] - m[10], m[15] - m[14]};

    for (FrustumPlane& plane : planes_)
    {
        plane = NormalizePlane(plane);
    }

    hasFrustum_ = true;
}

void GPUFrustumCulling::PerformCulling()
{
    visible.clear();
    visible.reserve(objects.size());

    if (!hasFrustum_)
    {
        // Preserve the original behavior when no camera/frustum has been set.
        for (std::size_t i = 0; i < objects.size(); ++i)
        {
            if (objects[i].radius > 0.0f)
            {
                visible.push_back(static_cast<int>(i));
            }
        }
        return;
    }

    for (std::size_t i = 0; i < objects.size(); ++i)
    {
        if (SphereInsideFrustum(objects[i], planes_))
        {
            visible.push_back(static_cast<int>(i));
        }
    }
}

void GPUFrustumCulling::PerformCulling(const float viewProjection[16])
{
    SetViewProjectionMatrix(viewProjection);
    PerformCulling();
}

const std::vector<int>& GPUFrustumCulling::VisibleObjects() const
{
    return visible;
}

FrustumPlane GPUFrustumCulling::NormalizePlane(FrustumPlane plane)
{
    const float lengthSquared = plane.x * plane.x +
                                plane.y * plane.y +
                                plane.z * plane.z;
    if (!(lengthSquared > 0.0f) || !std::isfinite(lengthSquared))
    {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }

    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    plane.x *= inverseLength;
    plane.y *= inverseLength;
    plane.z *= inverseLength;
    plane.d *= inverseLength;
    return plane;
}

bool GPUFrustumCulling::SphereInsideFrustum(
    const BoundingSphere& sphere,
    const std::array<FrustumPlane, 6>& planes)
{
    if (!std::isfinite(sphere.x) || !std::isfinite(sphere.y) ||
        !std::isfinite(sphere.z) || !std::isfinite(sphere.radius) ||
        sphere.radius < 0.0f)
    {
        return false;
    }

    for (const FrustumPlane& plane : planes)
    {
        const float distance = plane.x * sphere.x +
                               plane.y * sphere.y +
                               plane.z * sphere.z + plane.d;
        if (distance < -sphere.radius)
        {
            return false;
        }
    }

    return true;
}
