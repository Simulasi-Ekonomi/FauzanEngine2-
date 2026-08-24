#pragma once
#include "MathUtils.h"

struct Vector3 {
    float x, y, z;

    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float InX, float InY, float InZ) : x(InX), y(InY), z(InZ) {}

    Vector3 operator+(const Vector3& V) const { return Vector3(x + V.x, y + V.y, z + V.z); }
    Vector3 operator-(const Vector3& V) const { return Vector3(x - V.x, y - V.y, z - V.z); }
    Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }

    float SizeSquared() const { return x*x + y*y + z*z; }
    float Size() const { return Math::SafeSqrt(SizeSquared()); }

    bool Normalize(float Tolerance = Math::SMALL_NUMBER) {
        const float SquareSum = x*x + y*y + z*z;
        if (SquareSum > Tolerance) {
            const float Scale = 1.0f / Math::SafeSqrt(SquareSum);
            x *= Scale; y *= Scale; z *= Scale;
            return true;
        }
        return false;
    }

    static const Vector3 Zero;
};
