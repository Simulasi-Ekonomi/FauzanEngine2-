#pragma once
#include <vector>

struct LightProbe
{
    float x;
    float y;
    float z;
    float intensity;
};

class GlobalIllumination
{
public:

    void AddProbe(const LightProbe& p);

    void Compute();

private:

    std::vector<LightProbe> probes;
};
