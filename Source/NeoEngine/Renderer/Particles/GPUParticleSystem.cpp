#include "GPUParticleSystem.h"

void GPUParticleSystem::Spawn(const Particle& p)
{
    particles.push_back(p);
}

void GPUParticleSystem::Update(float dt)
{
    for(auto& p : particles)
    {
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.z += p.vz * dt;
    }
}
