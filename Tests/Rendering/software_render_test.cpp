#include "Source/NeoEngine/Rendering/Software/SoftwareRenderer.h"
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>

using namespace NeoEngine;

void CreateCube(std::vector<Vertex>& v, std::vector<unsigned>& idx) {
    v.clear();
    // 8 vertices
    v.push_back({{-1,-1, 1}});
    v.push_back({{ 1,-1, 1}});
    v.push_back({{ 1, 1, 1}});
    v.push_back({{-1, 1, 1}});
    v.push_back({{ 1,-1,-1}});
    v.push_back({{-1,-1,-1}});
    v.push_back({{-1, 1,-1}});
    v.push_back({{ 1, 1,-1}});

    idx = {0,1,2, 0,2,3, 4,5,6, 4,6,7,
           3,2,6, 3,6,5, 0,4,7, 0,7,1,
           0,3,5, 0,5,4, 1,7,6, 1,6,2};
}

void CreateSphere(std::vector<Vertex>& v, std::vector<unsigned>& idx, float r=1, int seg=24) {
    v.clear(); idx.clear();
    for(int y=0;y<=seg;y++) for(int x=0;x<=seg;x++) {
        float xs=(float)x/seg, ys=(float)y/seg;
        float px=cos(xs*2*M_PI)*sin(ys*M_PI), py=cos(ys*M_PI), pz=sin(xs*2*M_PI)*sin(ys*M_PI);
        v.push_back({{px*r,py*r,pz*r}});
    }
    for(int y=0;y<seg;y++) for(int x=0;x<seg;x++) {
        unsigned i0=y*(seg+1)+x, i1=i0+1, i2=i0+seg+1, i3=i2+1;
        idx.insert(idx.end(),{i0,i1,i2,i1,i3,i2});
    }
}

void CreateTorus(std::vector<Vertex>& v, std::vector<unsigned>& idx, float ir=0.5f, float orr=1.f, int sides=20, int rings=20) {
    v.clear(); idx.clear();
    for(int i=0;i<=rings;i++) {
        float th=i*2*M_PI/rings, ct=cos(th), st=sin(th);
        for(int j=0;j<=sides;j++) {
            float ph=j*2*M_PI/sides, cp=cos(ph), sp=sin(ph);
            float x=(orr+ir*cp)*ct, y=ir*sp, z=(orr+ir*cp)*st;
            v.push_back({{x,y,z}});
        }
    }
    for(int i=0;i<rings;i++) for(int j=0;j<sides;j++) {
        unsigned i0=i*(sides+1)+j, i1=i0+1, i2=i0+sides+1, i3=i2+1;
        idx.insert(idx.end(),{i0,i1,i2,i1,i3,i2});
    }
}

void CreatePyramid(std::vector<Vertex>& v, std::vector<unsigned>& idx) {
    v.clear();
    // Base
    v.push_back({{-1,0, 1}});
    v.push_back({{ 1,0, 1}});
    v.push_back({{ 1,0,-1}});
    v.push_back({{-1,0,-1}});
    // Tip
    v.push_back({{0,2,0}});
    idx = {0,1,2, 0,2,3, 0,1,4, 1,2,4, 2,3,4, 3,0,4};
}

void CreateCylinder(std::vector<Vertex>& v, std::vector<unsigned>& idx, float r=1, float h=2, int seg=24) {
    v.clear(); idx.clear();
    for(int i=0;i<=seg;i++) {
        float ang=i*2*M_PI/seg; float x=cos(ang)*r, z=sin(ang)*r;
        v.push_back({{x,-h/2,z}});
        v.push_back({{x, h/2,z}});
    }
    for(int i=0;i<seg;i++) {
        unsigned i0=i*2, i1=i*2+1, i2=i0+2, i3=i0+3;
        idx.insert(idx.end(),{i0,i1,i2,i1,i3,i2});
    }
}

void CreateCone(std::vector<Vertex>& v, std::vector<unsigned>& idx, float r=1, float h=2, int seg=24) {
    v.clear(); idx.clear();
    v.push_back({{0,h/2,0}});
    for(int i=0;i<=seg;i++) {
        float ang=i*2*M_PI/seg; float x=cos(ang)*r, z=sin(ang)*r;
        v.push_back({{x,-h/2,z}});
    }
    for(int i=0;i<seg;i++) idx.insert(idx.end(),{0,(unsigned)i+1,(unsigned)i+2});
}

int main() {
    SoftwareRenderer renderer(80, 40);
    const char* names[] = {"Cube","Sphere","Torus","Pyramid","Cylinder","Cone"};

    for(int i=0; i<6; ++i) {
        std::vector<Vertex> verts;
        std::vector<unsigned> indices;
        switch(i) {
            case 0: CreateCube(verts,indices); break;
            case 1: CreateSphere(verts,indices); break;
            case 2: CreateTorus(verts,indices); break;
            case 3: CreatePyramid(verts,indices); break;
            case 4: CreateCylinder(verts,indices); break;
            case 5: CreateCone(verts,indices); break;
        }

        printf("\n=== %s ===\n", names[i]);
        renderer.Clear();
        renderer.DrawMesh(verts, indices);
        renderer.Present();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    printf("\n✅ Semua 6 bentuk 3D berhasil ditampilkan dalam ASCII\n");
    return 0;
}
