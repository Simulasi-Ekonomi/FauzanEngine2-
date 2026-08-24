#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cmath>

namespace NeoEngine {

struct Vector3 { float x = 0, y = 0, z = 0; };

struct ActorComponent {
    std::string type;
    std::string name;
    std::function<void(float)> onUpdate;
    bool enabled = true;
};

class ActorCore {
public:
    ActorCore() = default;
    explicit ActorCore(const std::string& name) : m_Name(name) {}
    
    void SetName(const std::string& name) { m_Name = name; }
    const std::string& GetName() const { return m_Name; }
    
    void SetPosition(float x, float y, float z) { m_Pos = {x, y, z}; }
    Vector3 GetPosition() const { return m_Pos; }
    
    void SetRotation(float x, float y, float z) { m_Rot = {x, y, z}; }
    Vector3 GetRotation() const { return m_Rot; }
    
    void SetScale(float x, float y, float z) { m_Scale = {x, y, z}; }
    Vector3 GetScale() const { return m_Scale; }
    
    void Move(float dx, float dy, float dz) {
        m_Pos.x += dx; m_Pos.y += dy; m_Pos.z += dz;
    }
    
    void Rotate(float rx, float ry, float rz) {
        m_Rot.x += rx; m_Rot.y += ry; m_Rot.z += rz;
    }
    
    void AddComponent(const std::string& type, const std::string& name, std::function<void(float)> updateFn) {
        m_Components.push_back({type, name, updateFn});
    }
    
    void Update(float deltaTime) {
        for (auto& comp : m_Components) {
            if (comp.enabled && comp.onUpdate) comp.onUpdate(deltaTime);
        }
    }
    
    void SetVisible(bool v) { m_Visible = v; }
    bool IsVisible() const { return m_Visible; }
    
    float DistanceTo(const ActorCore& other) const {
        float dx = m_Pos.x - other.m_Pos.x;
        float dy = m_Pos.y - other.m_Pos.y;
        float dz = m_Pos.z - other.m_Pos.z;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }

private:
    std::string m_Name = "Actor";
    Vector3 m_Pos, m_Rot, m_Scale{1,1,1};
    bool m_Visible = true;
    std::vector<ActorComponent> m_Components;
};

} // namespace NeoEngine
