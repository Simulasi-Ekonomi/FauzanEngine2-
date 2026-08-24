#pragma once
#include <cstring>

struct Matrix4 {
    float m[16];

    Matrix4() { std::memset(m, 0, sizeof(m)); }

    static Matrix4 Identity() {
        Matrix4 res;
        res.m[0] = res.m[5] = res.m[10] = res.m[15] = 1.0f;
        return res;
    }

    // Row-major vs Column-major alignment untuk Vulkan/OpenGL
};
