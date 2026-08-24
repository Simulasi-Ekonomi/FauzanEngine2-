#include "Core/Math/NeoMath.h"
#pragma once
#include "MathDefines.h"

namespace Math {
    inline float SafeSqrt(float v) { return v <= 0.0f ? 0.0f : NeoEngine::Math::Sqrt(v); }

    inline bool IsNearlyZero(float Value, float ErrorTolerance = EPSILON) {
        return NeoEngine::Math::Fabs(Value) <= ErrorTolerance;
    }

    inline float ToRadians(float Degrees) { return Degrees * (PI / 180.0f); }
    inline float ToDegrees(float Radians) { return Radians * (180.0f / PI); }
}
