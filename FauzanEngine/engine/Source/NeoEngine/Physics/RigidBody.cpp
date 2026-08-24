#include <cassert>
#include "RigidBody.h"

namespace NeoEngine
{

RigidBody::RigidBody(float m)
{
    mass = m;

    position = {0,0,0};
    velocity = {0,0,0};
    force = {0,0,0};
}

void RigidBody::ApplyForce(const Vec3& f)
{
    force.x += f.x;
    force.y += f.y;
    force.z += f.z;
}

void RigidBody::Integrate(float dt)
{
    Vec3 acceleration;

    acceleration.x = force.x / mass;
    acceleration.y = force.y / mass;
    acceleration.z = force.z / mass;

    velocity.x += acceleration.x * dt;
    velocity.y += acceleration.y * dt;
    velocity.z += acceleration.z * dt;

    position.x += velocity.x * dt;
    position.y += velocity.y * dt;
    position.z += velocity.z * dt;

    force = {0,0,0};
}

Vec3 RigidBody::GetPosition() const
{
    return position;
}

Vec3 RigidBody::GetVelocity() const
{
    return velocity;
}

void RigidBody::SetPosition(float x,float y,float z)
{
    position = {x,y,z};
}

}
