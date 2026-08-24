#pragma once
#include <vector>

struct RigidBody
{
    float x;
    float y;
    float z;

    float vx;
    float vy;
    float vz;
};

class DestructionPhysicsSolver
{
public:

    void AddBody(const RigidBody& body);
    void Step(float dt);

private:

    std::vector<RigidBody> bodies;
};
