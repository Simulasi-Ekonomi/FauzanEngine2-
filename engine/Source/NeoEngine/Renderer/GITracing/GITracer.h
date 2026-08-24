#pragma once
#include <vector>

struct GITraceRay
{
    float ox;
    float oy;
    float oz;

    float dx;
    float dy;
    float dz;
};

class GITracer
{
public:

    void AddRay(const GITraceRay& r);

    void Trace();

private:

    std::vector<GITraceRay> rays;
};
