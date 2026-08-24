#pragma once
#include <vector>

struct Particle
{
    float x;
    float y;
    float z;
    float vx;
    float vy;
    float vz;
};

class GPUParticleSystem
{
public:

    void Spawn(const Particle& p);
    void Update(float dt);

private:

    std::vector<Particle> particles;
};
