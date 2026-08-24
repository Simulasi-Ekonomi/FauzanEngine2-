#pragma once
#include <vector>

struct GIProbe
{
    float x;
    float y;
    float z;
    float energy;
};

class DynamicGI
{
public:

    void AddProbe(const GIProbe& p);

    void UpdateLighting();

private:

    std::vector<GIProbe> probes;
};
