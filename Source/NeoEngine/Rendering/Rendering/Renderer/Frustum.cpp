#include "Core/Math/NeoMath.h"
#include <cassert>
#include "Frustum.h"
#include <cmath>

void Frustum::NormalizePlane(Plane& p)
{
    float mag = NeoEngine::Math::Sqrt(p.a*p.a + p.b*p.b + p.c*p.c);

    p.a /= mag;
    p.b /= mag;
    p.c /= mag;
    p.d /= mag;
}
