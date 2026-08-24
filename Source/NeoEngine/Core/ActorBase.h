#pragma once

#include "Core/Math/Vector3.h"
#include "Core/Math/Quaternion.h"

#include <string>
#include <vector>
#include <functional>
#include "Core/Math/NeoMath.h"

namespace NeoEngine {

class CharacterBase;

// Delegasi multicast (tanpa <memory>)
template<typename... Args>
class TMulticastDelegate {
public:
    void Add(std::function<void(Args...)> func) { callbacks.push_back(func); }
    void Broadcast(Args... args) { for (auto& cb : callbacks) cb(args...); }
private:
    std::vector<std::function<void(Args...)>> callbacks;
};

// Kelas dasar mirip AActor
class ActorBase {
public:
    ActorBase() = default;
    virtual ~ActorBase() = default;

    void SetActorLocation(const Vector3& loc) { m_Location = loc; }
    Vector3 GetActorLocation() const { return m_Location; }
    Vector3 GetLocation() const { return m_Location; }

    void SetActorRotation(const Quaternion& rot) { m_Rotation = rot; }
    Quaternion GetActorRotation() const { return m_Rotation; }

    void SetActorScale(const Vector3& scale) { m_Scale = scale; }
    Vector3 GetActorScale() const { return m_Scale; }

    void AddWorldOffset(const Vector3& offset) { m_Location = m_Location + offset; }
    void AddLocalOffset(const Vector3& offset) { m_Location = m_Location + offset; }
    void AddActorWorldRotation(const Quaternion& delta) { m_Rotation = m_Rotation * delta; }

    Vector3 GetForwardVector() const { return RotateVector(Vector3(1,0,0)); }
    Vector3 GetRightVector() const   { return RotateVector(Vector3(0,1,0)); }
    Vector3 GetUpVector() const      { return RotateVector(Vector3(0,0,1)); }

    virtual void BeginPlay() {}
    virtual void Tick(float deltaTime) {}
    virtual void EndPlay() {}

    void SetActorName(const std::string& name) { m_Name = name; }
    std::string GetActorName() const { return m_Name; }
    std::string GetName() const { return m_Name; }
    bool IsValid() const { return true; }

protected:
    Vector3 m_Location;
    Vector3 m_Scale{1.0f, 1.0f, 1.0f};
    Quaternion m_Rotation;
    std::string m_Name;
    TMulticastDelegate<float> OnTickDelegate;

private:
    Vector3 RotateVector(const Vector3& v) const {
        float rx = m_Rotation.w*v.x + m_Rotation.y*v.z - m_Rotation.z*v.y;
        float ry = m_Rotation.w*v.y + m_Rotation.z*v.x - m_Rotation.x*v.z;
        float rz = m_Rotation.w*v.z + m_Rotation.x*v.y - m_Rotation.y*v.x;
        float rw = -m_Rotation.x*v.x - m_Rotation.y*v.y - m_Rotation.z*v.z;
        return Vector3(
            rx*m_Rotation.w - rw*m_Rotation.x - ry*m_Rotation.z + rz*m_Rotation.y,
            ry*m_Rotation.w - rw*m_Rotation.y - rz*m_Rotation.x + rx*m_Rotation.z,
            rz*m_Rotation.w - rw*m_Rotation.z - rx*m_Rotation.y + ry*m_Rotation.x
        );
    }
};

} // namespace NeoEngine
