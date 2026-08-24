#include "GlobalIllumination.h"

void GlobalIllumination::AddProbe(const LightProbe& p)
{
    probes.push_back(p);
}

void GlobalIllumination::Compute()
{
    for(auto& p : probes)
    {
        p.intensity *= 0.95f;
    }
}
