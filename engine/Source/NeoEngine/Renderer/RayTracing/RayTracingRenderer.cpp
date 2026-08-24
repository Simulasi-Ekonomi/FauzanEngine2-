#include "RayTracingRenderer.h"

void RayTracingRenderer::AddRay(const Ray& ray)
{
    rays.push_back(ray);
}

void RayTracingRenderer::Trace()
{
    hits.clear();

    for(auto& r : rays)
    {
        HitInfo h;
        h.distance = 1.0f;

        hits.push_back(h);
    }
}
