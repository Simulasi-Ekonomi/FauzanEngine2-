#pragma once
#ifndef VEC4_H
#define VEC4_H

struct Vec4 {
    union {
        struct { float x, y, z, w; };
        float m[4];
    };

    // Constructors
    constexpr Vec4() : x(0), y(0), z(0), w(0) {}
    constexpr Vec4(float xx, float yy, float zz, float ww) : x(xx), y(yy), z(zz), w(ww) {}
};

#endif // VEC4_H