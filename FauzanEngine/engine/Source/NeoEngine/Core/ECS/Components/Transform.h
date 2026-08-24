#pragma once
#include <cmath>

namespace NeoEngine
{

struct Vec3
{
    float x = 0, y = 0, z = 0;

    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s)       const { return {x*s,   y*s,   z*s};   }
    Vec3 operator-()              const { return {-x, -y, -z};           }
    Vec3& operator+=(const Vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x-=o.x; y-=o.y; z-=o.z; return *this; }

    float dot(const Vec3& o) const { return x*o.x + y*o.y + z*o.z; }

    float length() const { return std::sqrt(x*x + y*y + z*z); }

    Vec3 normalized() const
    {
        float l = length();
        if (l < 1e-6f) return {0,0,0};
        return {x/l, y/l, z/l};
    }

    void Normalize() { *this = normalized(); }
};

struct Quat
{
    float x = 0, y = 0, z = 0, w = 1;

    Quat() = default;
    Quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    static Quat Identity() { return {0,0,0,1}; }

    static Quat FromAxisAngle(const Vec3& axis, float angle)
    {
        float s = std::sin(angle * 0.5f);
        float c = std::cos(angle * 0.5f);
        Vec3  a = axis.normalized();
        return {a.x*s, a.y*s, a.z*s, c};
    }

    Quat operator*(const Quat& o) const
    {
        return {
            w*o.x + x*o.w + y*o.z - z*o.y,
            w*o.y - x*o.z + y*o.w + z*o.x,
            w*o.z + x*o.y - y*o.x + z*o.w,
            w*o.w - x*o.x - y*o.y - z*o.z
        };
    }

    Vec3 RotateVector(const Vec3& v) const
    {
        // q * v * q^-1
        Vec3 u{x, y, z};
        float s = w;
        return u * (2.0f * u.dot(v))
             + v * (s*s - u.dot(u))
             + Vec3{
                 u.y*v.z - u.z*v.y,
                 u.z*v.x - u.x*v.z,
                 u.x*v.y - u.y*v.x
               } * (2.0f * s);
    }

    void Normalize()
    {
        float l = std::sqrt(x*x + y*y + z*z + w*w);
        if (l < 1e-6f) { *this = Identity(); return; }
        x/=l; y/=l; z/=l; w/=l;
    }
};

struct Mat4
{
    float m[16] = {};

    static Mat4 Identity()
    {
        Mat4 r{};
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }
};

struct Transform
{
    Vec3 position {0,0,0};
    Quat rotation {};
    Vec3 scale    {1,1,1};
    Mat4 model;

    void UpdateMatrix()
    {
        // Rotation dari Quaternion
        float xx=rotation.x*rotation.x, yy=rotation.y*rotation.y;
        float zz=rotation.z*rotation.z, xy=rotation.x*rotation.y;
        float xz=rotation.x*rotation.z, yz=rotation.y*rotation.z;
        float wx=rotation.w*rotation.x, wy=rotation.w*rotation.y;
        float wz=rotation.w*rotation.z;

        Mat4 r = Mat4::Identity();
        r.m[0]=1-2*(yy+zz); r.m[1]=2*(xy+wz);   r.m[2]=2*(xz-wy);
        r.m[4]=2*(xy-wz);   r.m[5]=1-2*(xx+zz); r.m[6]=2*(yz+wx);
        r.m[8]=2*(xz+wy);   r.m[9]=2*(yz-wx);   r.m[10]=1-2*(xx+yy);

        // Scale
        r.m[0]*=scale.x; r.m[1]*=scale.x; r.m[2]*=scale.x;
        r.m[4]*=scale.y; r.m[5]*=scale.y; r.m[6]*=scale.y;
        r.m[8]*=scale.z; r.m[9]*=scale.z; r.m[10]*=scale.z;

        // Translation
        r.m[12]=position.x; r.m[13]=position.y; r.m[14]=position.z;
        r.m[15]=1.0f;

        model = r;
    }
};

} // namespace NeoEngine
