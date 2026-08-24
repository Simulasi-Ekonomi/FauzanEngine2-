#include "Core/Math/NeoMath.h"
#pragma once
#include <string>

namespace NeoEngine {

enum class ProjectionType { Perspective, Orthographic };

class CameraManagerCore {
public:
    CameraManagerCore() { UpdateMatrices(); }
    
    void SetPosition(float x, float y, float z) { m_PosX = x; m_PosY = y; m_PosZ = z; UpdateView(); }
    void SetTarget(float x, float y, float z) { m_TargetX = x; m_TargetY = y; m_TargetZ = z; UpdateView(); }
    void SetFOV(float fov) { m_FOV = fov; UpdateProjection(); }
    void SetNearFar(float n, float f) { m_Near = n; m_Far = f; UpdateProjection(); }
    void SetAspectRatio(float ar) { m_Aspect = ar; UpdateProjection(); }
    
    void MoveForward(float amount) {
        float dx = m_TargetX - m_PosX, dz = m_TargetZ - m_PosZ;
        float len = NeoEngine::Math::Sqrt(dx*dx + dz*dz);
        if (len > 0) { dx /= len; dz /= len; }
        m_PosX += dx * amount; m_PosZ += dz * amount;
        m_TargetX += dx * amount; m_TargetZ += dz * amount;
        UpdateView();
    }
    
    void RotateYaw(float degrees) {
        float rad = degrees * 3.14159f / 180.0f;
        float dx = m_TargetX - m_PosX, dz = m_TargetZ - m_PosZ;
        float newDx = dx * NeoEngine::Math::Cos(rad) - dz * NeoEngine::Math::Sin(rad);
        float newDz = dx * NeoEngine::Math::Sin(rad) + dz * NeoEngine::Math::Cos(rad);
        m_TargetX = m_PosX + newDx;
        m_TargetZ = m_PosZ + newDz;
        UpdateView();
    }
    
    const float* GetViewMatrix() const { return m_ViewMatrix; }
    const float* GetProjectionMatrix() const { return m_ProjMatrix; }
    const float* GetVPMatrix() const { return m_VPMatrix; }

private:
    void UpdateView() {
        float fx = m_TargetX - m_PosX, fy = m_TargetY - m_PosY, fz = m_TargetZ - m_PosZ;
        float len = NeoEngine::Math::Sqrt(fx*fx + fy*fy + fz*fz);
        fx /= len; fy /= len; fz /= len;
        float sx = fy * 0 - fz * 1, sy = fz * 0 - fx * 0, sz = fx * 1 - fy * 0; // simplified cross
        float ux = sy * fz - sz * fy, uy = sz * fx - sx * fz, uz = sx * fy - sy * fx;
        FillIdentity(m_ViewMatrix);
        m_ViewMatrix[0] = sx; m_ViewMatrix[4] = sy; m_ViewMatrix[8] = sz;
        m_ViewMatrix[1] = ux; m_ViewMatrix[5] = uy; m_ViewMatrix[9] = uz;
        m_ViewMatrix[2] = -fx; m_ViewMatrix[6] = -fy; m_ViewMatrix[10] = -fz;
        m_ViewMatrix[12] = -(sx*m_PosX + sy*m_PosY + sz*m_PosZ);
        m_ViewMatrix[13] = -(ux*m_PosX + uy*m_PosY + uz*m_PosZ);
        m_ViewMatrix[14] = (fx*m_PosX + fy*m_PosY + fz*m_PosZ);
        UpdateVPMatrix();
    }
    
    void UpdateProjection() {
        FillIdentity(m_ProjMatrix);
        float f = 1.0f / NeoEngine::Math::Tan(m_FOV * 3.14159f / 360.0f);
        m_ProjMatrix[0] = f / m_Aspect;
        m_ProjMatrix[5] = f;
        m_ProjMatrix[10] = (m_Far + m_Near) / (m_Near - m_Far);
        m_ProjMatrix[11] = -1;
        m_ProjMatrix[14] = (2 * m_Far * m_Near) / (m_Near - m_Far);
        UpdateVPMatrix();
    }
    
    void UpdateVPMatrix() { MultiplyMatrices(m_VPMatrix, m_ProjMatrix, m_ViewMatrix); }
    
    void FillIdentity(float* m) {
        for (int i = 0; i < 16; i++) m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
    
    void MultiplyMatrices(float* out, const float* a, const float* b) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                out[i*4+j] = a[i*4+0]*b[0*4+j] + a[i*4+1]*b[1*4+j] + a[i*4+2]*b[2*4+j] + a[i*4+3]*b[3*4+j];
            }
        }
    }
    
    void UpdateMatrices() { UpdateView(); UpdateProjection(); }
    
    float m_PosX = 0, m_PosY = 2, m_PosZ = 8;
    float m_TargetX = 0, m_TargetY = 0, m_TargetZ = 0;
    float m_FOV = 60, m_Near = 0.1f, m_Far = 1000, m_Aspect = 1.777f;
    float m_ViewMatrix[16], m_ProjMatrix[16], m_VPMatrix[16];
};

} // namespace NeoEngine
