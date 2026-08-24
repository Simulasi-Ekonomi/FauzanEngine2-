#pragma once
#include <vector>

namespace NeoEngine {

struct PhysicsBody {
    float posX=0, posY=0, posZ=0;
    float velX=0, velY=0, velZ=0;
    float mass=1.0f;
    bool isStatic=false;
    bool isKinematic=false;
    float friction=0.5f;
    float restitution=0.3f;
};

class PhysicsEngine {
public:
    void AddBody(const PhysicsBody& body) { m_Bodies.push_back(body); }
    void Step(float dt);
    void Clear() { m_Bodies.clear(); }
    size_t GetBodyCount() const { return m_Bodies.size(); }
    const std::vector<PhysicsBody>& GetBodies() const { return m_Bodies; }
private:
    std::vector<PhysicsBody> m_Bodies;
};

} // namespace NeoEngine
