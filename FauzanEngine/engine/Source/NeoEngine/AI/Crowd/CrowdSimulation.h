#pragma once
#include <vector>

struct CrowdAgent
{
    float x;
    float y;
    float z;

    float vx;
    float vy;
};

class CrowdSimulation
{
public:

    void AddAgent(const CrowdAgent& a);

    void Update(float dt);

private:

    std::vector<CrowdAgent> agents;
};
