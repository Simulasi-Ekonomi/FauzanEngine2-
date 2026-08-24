#include "Source/NeoEngine/Rendering/Software/SoftwareRenderer.h"
using namespace NeoEngine;

void AddBox(std::vector<Vertex>& v, std::vector<unsigned>& idx,
            float x, float y, float z, float w, float h, float d) {
    unsigned base = v.size();
    v.push_back({{x-w/2,y-h/2,z+d/2}}); v.push_back({{x+w/2,y-h/2,z+d/2}});
    v.push_back({{x+w/2,y+h/2,z+d/2}}); v.push_back({{x-w/2,y+h/2,z+d/2}});
    v.push_back({{x+w/2,y-h/2,z-d/2}}); v.push_back({{x-w/2,y-h/2,z-d/2}});
    v.push_back({{x-w/2,y+h/2,z-d/2}}); v.push_back({{x+w/2,y+h/2,z-d/2}});
    unsigned b=base;
    unsigned faces[]={0,1,2,0,2,3,4,5,6,4,6,7,3,2,6,3,6,5,0,4,7,0,7,1,0,3,5,0,5,4,1,7,6,1,6,2};
    for(int i=0;i<36;i++) idx.insert(idx.end(),{b+faces[i],b+faces[i+1],b+faces[i+2]});
}

int main() {
    SoftwareRenderer renderer(100, 45);
    
    // Kamera: agak jauh dan tinggi
    Mat4 proj = Mat4::Perspective(60.0f*M_PI/180.0f, 100.0f/45.0f, 0.1f, 50.0f);
    Mat4 view = Mat4::LookAt({0,3,8}, {0,1,0}, {0,1,0});
    renderer.SetTransform(proj * view);
    
    std::vector<Vertex> verts;
    std::vector<unsigned> indices;
    
    // Grid lantai
    for(int x=-5; x<=5; ++x) {
        verts.push_back({{(float)x, 0, -5}});
        verts.push_back({{(float)x, 0,  5}});
        indices.push_back(verts.size()-2);
        indices.push_back(verts.size()-1);
    }
    for(int z=-5; z<=5; ++z) {
        verts.push_back({{-5, 0, (float)z}});
        verts.push_back({{ 5, 0, (float)z}});
        indices.push_back(verts.size()-2);
        indices.push_back(verts.size()-1);
    }
    
    // Mobil (kotak rendah)
    AddBox(verts, indices, 0.0f, 0.6f, 0.0f, 3.0f, 0.8f, 1.5f);
    // Atap mobil
    AddBox(verts, indices, 0.0f, 1.4f, 0.0f, 1.8f, 0.6f, 1.2f);
    
    // Karakter (kotak vertikal)
    AddBox(verts, indices, 3.0f, 0.8f, 0.0f, 0.5f, 1.2f, 0.5f);
    // Kepala
    AddBox(verts, indices, 3.0f, 1.8f, 0.0f, 0.4f, 0.4f, 0.4f);
    
    renderer.Clear();
    renderer.DrawMesh(verts, indices);
    renderer.Present();
    return 0;
}
