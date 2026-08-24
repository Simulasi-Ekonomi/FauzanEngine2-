#pragma once

#include <array>

namespace NeoEngine
{

struct Vec3
{
    [[maybe_unused]] float x;
    [[maybe_unused]] float y;
    [[maybe_unused]] float z;
};

class RigidBody
{
public:

    RigidBody(float mass);

    void ApplyForce(const Vec3& f);
    void Integrate(float dt);

    Vec3 GetPosition() const;
    Vec3 GetVelocity() const;

    void SetPosition(float x,float y,float z);

private:

    [[maybe_unused]] float mass;

    Vec3 position;
    Vec3 velocity;
    Vec3 force;

};

}
