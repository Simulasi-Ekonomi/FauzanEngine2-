#pragma once
#include <cmath>
#include "MathDefines.h"

namespace Math {
    inline float SafeSqrt(float v) { return v <= 0.0f ? 0.0f : std::sqrt(v); }

    inline bool IsNearlyZero(float Value, float ErrorTolerance = EPSILON) {
        return std::abs(Value) <= ErrorTolerance;
    }

    inline float ToRadians(float Degrees) { return Degrees * (PI / 180.0f); }
    inline float ToDegrees(float Radians) { return Radians * (180.0f / PI); }
}
