#include "CrowdSimulation.h"

void CrowdSimulation::AddAgent(const CrowdAgent& a)
{
    agents.push_back(a);
}

void CrowdSimulation::Update(float dt)
{
    for(auto& a : agents)
    {
        a.x += a.vx * dt;
        a.y += a.vy * dt;
    }
}
