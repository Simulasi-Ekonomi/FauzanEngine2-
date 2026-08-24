#include "DynamicGI.h"

void DynamicGI::AddProbe(const GIProbe& p)
{
    probes.push_back(p);
}

void DynamicGI::UpdateLighting()
{
    for(auto& p : probes)
    {
        p.energy *= 0.97f;
    }
}
