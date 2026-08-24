#include "GITracer.h"

void GITracer::AddRay(const GITraceRay& r)
{
    rays.push_back(r);
}

void GITracer::Trace()
{
    for(auto& r : rays)
    {
        r.dx *= 0.98f;
    }
}
