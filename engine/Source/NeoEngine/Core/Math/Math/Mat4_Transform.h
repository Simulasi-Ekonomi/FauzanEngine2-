#pragma once

#include "../ECS/Components/Transform.h"
#include <cmath>

namespace NeoEngine
{

inline Mat4 Translate(float x, float y, float z)
{
    Mat4 m = Mat4::Identity();
    m.m[12]=x; m.m[13]=y; m.m[14]=z;
    return m;
}

inline Mat4 Scale(float sx, float sy, float sz)
{
    Mat4 m = Mat4::Identity();
    m.m[0]=sx; m.m[5]=sy; m.m[10]=sz;
    return m;
}

inline Mat4 Multiply(const Mat4& a, const Mat4& b)
{
    Mat4 r{};
    for (int row=0; row<4; row++)
        for (int col=0; col<4; col++)
            r.m[row*4+col] =
                a.m[row*4+0]*b.m[col+0] +
                a.m[row*4+1]*b.m[col+4] +
                a.m[row*4+2]*b.m[col+8] +
                a.m[row*4+3]*b.m[col+12];
    return r;
}

inline Mat4 FromQuat(const Quat& q)
{
    Mat4 m = Mat4::Identity();
    float xx=q.x*q.x, yy=q.y*q.y, zz=q.z*q.z;
    float xy=q.x*q.y, xz=q.x*q.z, yz=q.y*q.z;
    float wx=q.w*q.x, wy=q.w*q.y, wz=q.w*q.z;
    m.m[0]=1-2*(yy+zz); m.m[1]=2*(xy+wz);   m.m[2]=2*(xz-wy);
    m.m[4]=2*(xy-wz);   m.m[5]=1-2*(xx+zz); m.m[6]=2*(yz+wx);
    m.m[8]=2*(xz+wy);   m.m[9]=2*(yz-wx);   m.m[10]=1-2*(xx+yy);
    return m;
}

inline Mat4 Perspective(float fov, float aspect, float nearP, float farP)
{
    Mat4 m{};
    float t = std::tan(fov * 0.5f * 3.14159265f / 180.0f);
    m.m[0]  =  1.0f/(aspect*t);
    m.m[5]  =  1.0f/t;
    m.m[10] = -(farP+nearP)/(farP-nearP);
    m.m[11] = -1.0f;
    m.m[14] = -(2.0f*farP*nearP)/(farP-nearP);
    return m;
}

inline Mat4 InverseTransform(const Vec3& pos, const Quat& rot)
{
    Quat inv{-rot.x, -rot.y, -rot.z, rot.w};
    return Multiply(FromQuat(inv), Translate(-pos.x, -pos.y, -pos.z));
}

} // namespace NeoEngine
