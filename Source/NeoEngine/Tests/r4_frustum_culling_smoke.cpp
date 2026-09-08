#include "Renderer/GPUFrustumCulling.h"

#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    GPUFrustumCulling culling;

    // Identity clip volume: x/y/z in [-1, +1].
    const float identity[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    culling.AddObject({0.0f, 0.0f, 0.0f, 0.10f}); // inside
    culling.AddObject({1.20f, 0.0f, 0.0f, 0.10f}); // outside right
    culling.AddObject({-1.05f, 0.0f, 0.0f, 0.10f}); // intersects left plane
    culling.AddObject({0.0f, 0.0f, -1.50f, 0.10f}); // outside near
    culling.AddObject({0.0f, 0.0f, 1.05f, 0.10f}); // intersects far plane

    culling.PerformCulling(identity);

    const auto& visible = culling.VisibleObjects();
    assert(culling.HasFrustum());
    assert(visible.size() == 3);
    assert(visible[0] == 0);
    assert(visible[1] == 2);
    assert(visible[2] == 4);

    // Invalid spheres must never become visible.
    GPUFrustumCulling invalid;
    invalid.AddObject({0.0f, 0.0f, 0.0f, -1.0f});
    invalid.AddObject({NAN, 0.0f, 0.0f, 1.0f});
    invalid.PerformCulling(identity);
    assert(invalid.VisibleObjects().empty());

    // Legacy no-camera contract remains supported.
    GPUFrustumCulling legacy;
    legacy.AddObject({0.0f, 0.0f, 0.0f, 1.0f});
    legacy.AddObject({0.0f, 0.0f, 0.0f, 0.0f});
    legacy.AddObject({0.0f, 0.0f, 0.0f, -1.0f});
    legacy.PerformCulling();
    assert(legacy.VisibleObjects().size() == 1);
    assert(legacy.VisibleObjects()[0] == 0);

    std::cout << "R4_FRUSTUM_OK visible=" << visible.size() << " planes=6\n";
    return 0;
}
