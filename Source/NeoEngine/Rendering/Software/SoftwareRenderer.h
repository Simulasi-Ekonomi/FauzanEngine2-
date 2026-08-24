#pragma once
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace NeoEngine {

struct Vec3 { float x,y,z;
    Vec3()=default;
    Vec3(float x,float y,float z):x(x),y(y),z(z){}
    Vec3 operator-(const Vec3&o)const{return {x-o.x,y-o.y,z-o.z};}
    Vec3 operator+(const Vec3&o)const{return {x+o.x,y+o.y,z+o.z};}
    Vec3 operator*(float s)const{return {x*s,y*s,z*s};}
    Vec3 Cross(const Vec3&o)const{return {y*o.z-z*o.y,z*o.x-x*o.z,x*o.y-y*o.x};}
    float Dot(const Vec3&o)const{return x*o.x+y*o.y+z*o.z;}
    Vec3 Normalized()const{float l=sqrtf(x*x+y*y+z*z);return l>1e-6f?Vec3{x/l,y/l,z/l}:Vec3{};}
};

struct Vertex { Vec3 pos; };

struct Mat4 {
    float m[16];
    Mat4(){ Identity(); }
    void Identity(){ for(int i=0;i<16;++i) m[i]=(i%5==0)?1.0f:0.0f; }
    
    static Mat4 Perspective(float fov, float aspect, float nearZ, float farZ) {
        Mat4 r; r.m[0]=1.0f/(aspect*tanf(fov*0.5f)); r.m[5]=1.0f/tanf(fov*0.5f);
        r.m[10]=-(farZ+nearZ)/(farZ-nearZ); r.m[11]=-1.0f;
        r.m[14]=-2.0f*farZ*nearZ/(farZ-nearZ); r.m[15]=0.0f;
        return r;
    }
    
    static Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f=(center-eye).Normalized();
        Vec3 s=f.Cross(up).Normalized();
        Vec3 u=s.Cross(f);
        Mat4 r;
        r.m[0]=s.x; r.m[4]=s.y; r.m[8]=s.z; r.m[12]=-s.Dot(eye);
        r.m[1]=u.x; r.m[5]=u.y; r.m[9]=u.z; r.m[13]=-u.Dot(eye);
        r.m[2]=-f.x; r.m[6]=-f.y; r.m[10]=-f.z; r.m[14]=f.Dot(eye);
        return r;
    }
    
    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for(int i=0;i<4;++i) for(int j=0;j<4;++j) {
            r.m[i*4+j]=0;
            for(int k=0;k<4;++k) r.m[i*4+j]+=m[i*4+k]*o.m[k*4+j];
        }
        return r;
    }
    
    Vec3 Transform(const Vec3& v) const {
        float x=m[0]*v.x+m[4]*v.y+m[8]*v.z+m[12];
        float y=m[1]*v.x+m[5]*v.y+m[9]*v.z+m[13];
        float z=m[2]*v.x+m[6]*v.y+m[10]*v.z+m[14];
        float w=m[3]*v.x+m[7]*v.y+m[11]*v.z+m[15];
        if(fabsf(w)<1e-6f) w=1e-6f;
        return {x/w, y/w, z/w};
    }
};

class SoftwareRenderer {
public:
    SoftwareRenderer(int w=80, int h=40):m_W(w),m_H(h){
        m_Buffer.assign(w*h, ' ');
        m_Z.assign(w*h, 1e9f);
    }

    void Clear() {
        std::fill(m_Buffer.begin(), m_Buffer.end(), ' ');
        std::fill(m_Z.begin(), m_Z.end(), 1e9f);
    }

    void SetTransform(const Mat4& mvp) { m_MVP = mvp; }

    void DrawLine(const Vec3& a, const Vec3& b) {
        Vec3 ta = m_MVP.Transform(a);
        Vec3 tb = m_MVP.Transform(b);
        
        if(ta.z < -1.0f || ta.z > 1.0f || tb.z < -1.0f || tb.z > 1.0f) return;
        
        int x0 = (int)((ta.x+1.0f)*0.5f*m_W);
        int y0 = (int)((1.0f-ta.y)*0.5f*m_H);
        int x1 = (int)((tb.x+1.0f)*0.5f*m_W);
        int y1 = (int)((1.0f-tb.y)*0.5f*m_H);
        
        x0 = std::clamp(x0, 0, m_W-1); y0 = std::clamp(y0, 0, m_H-1);
        x1 = std::clamp(x1, 0, m_W-1); y1 = std::clamp(y1, 0, m_H-1);
        
        int dx=abs(x1-x0), sx=x0<x1?1:-1;
        int dy=-abs(y1-y0), sy=y0<y1?1:-1;
        int err=dx+dy, e2;
        float z0=ta.z, z1=tb.z;
        int steps = std::max(abs(x1-x0)+abs(y1-y0), 1);
        float dz=(z1-z0)/steps;
        float z=z0;
        
        while(true) {
            if(z < m_Z[y0*m_W+x0]) {
                m_Z[y0*m_W+x0] = z;
                m_Buffer[y0*m_W+x0] = '#';
            }
            if(x0==x1 && y0==y1) break;
            e2=2*err;
            if(e2>=dy){err+=dy;x0+=sx;}
            if(e2<=dx){err+=dx;y0+=sy;}
            z += dz;
        }
    }

    void DrawMesh(const std::vector<Vertex>& verts, const std::vector<unsigned>& indices) {
        size_t n = verts.size();
        for(size_t i=0; i+2<indices.size(); i+=3) {
            unsigned i0=indices[i], i1=indices[i+1], i2=indices[i+2];
            if(i0>=n || i1>=n || i2>=n) continue;
            DrawLine(verts[i0].pos, verts[i1].pos);
            DrawLine(verts[i1].pos, verts[i2].pos);
            DrawLine(verts[i2].pos, verts[i0].pos);
        }
    }

    void Present() {
        for(int y=0; y<m_H; ++y) {
            for(int x=0; x<m_W; ++x)
                putchar(m_Buffer[y*m_W+x]);
            putchar('\n');
        }
    }

private:
    int m_W, m_H;
    std::vector<char> m_Buffer;
    std::vector<float> m_Z;
    Mat4 m_MVP;
};

}
