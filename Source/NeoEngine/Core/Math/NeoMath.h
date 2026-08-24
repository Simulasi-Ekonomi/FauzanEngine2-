#pragma once
namespace NeoEngine {
    struct Math {
        static float Sqrt(float x) {
            if (x <= 0) return 0;
            float r = x;
            for (int i = 0; i < 12; ++i) r = (r + x / r) * 0.5f;
            return r;
        }
        static float Sin(float x) {
            float x2 = x * x, term = x, res = x;
            for (int i = 3; i < 12; i += 2) {
                term *= -x2 / (i * (i - 1));
                res += term;
            }
            return res;
        }
        static float Cos(float x) { return Sin(1.57079632679f - x); }
        static float Tan(float x) { return Sin(x) / Cos(x); }
        static float Atan(float x) {
            float x2 = x * x, term = x, res = x;
            for (int i = 3; i < 15; i += 2) {
                term *= -x2;
                res += term / i;
            }
            return res;
        }
        static float Atan2(float y, float x) {
            if (x == 0) return (y > 0) ? 1.570796f : -1.570796f;
            return (Fabs(x) > Fabs(y)) ? Atan(y / x) :
                   (y > 0 ? 1.570796f - Atan(x / y) : -1.570796f - Atan(x / y));
        }
        static float Fabs(float x) { return x < 0 ? -x : x; }
    };
}
