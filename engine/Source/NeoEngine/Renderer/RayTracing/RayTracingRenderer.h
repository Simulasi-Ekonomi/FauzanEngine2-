#pragma once
#include <vector>

struct Ray
{
    float ox;
    float oy;
    float oz;

    float dx;
    float dy;
    float dz;
};

struct HitInfo
{
    float distance;
};

class RayTracingRenderer
{
public:

    void AddRay(const Ray& ray);

    void Trace();

private:

    std::vector<Ray> rays;
    std::vector<HitInfo> hits;
};
