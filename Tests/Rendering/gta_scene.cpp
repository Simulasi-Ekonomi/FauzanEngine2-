#include "Source/NeoEngine/Rendering/Software/SoftwareRenderer.h"
using namespace NeoEngine;

// Fungsi bantu untuk membuat kotak
void AddBox(std::vector<Vertex>& v, std::vector<unsigned>& idx,
            float x, float y, float z, float w, float h, float d) {
    unsigned base = v.size();
    v.insert(v.end(), {
        {{x-w/2, y-h/2, z+d/2}}, {{x+w/2, y-h/2, z+d/2}},
        {{x+w/2, y+h/2, z+d/2}}, {{x-w/2, y+h/2, z+d/2}},
        {{x+w/2, y-h/2, z-d/2}}, {{x-w/2, y-h/2, z-d/2}},
        {{x-w/2, y+h/2, z-d/2}}, {{x+w/2, y+h/2, z-d/2}}
    });
    unsigned b=base;
    unsigned faces[] = {0,1,2,0,2,3, 4,5,6,4,6,7, 3,2,6,3,6,5,
                        0,4,7,0,7,1, 0,3,5,0,5,4, 1,7,6,1,6,2};
    for(int i=0;i<36;i+=3) idx.insert(idx.end(),{b+faces[i],b+faces[i+1],b+faces[i+2]});
}

int main() {
    SoftwareRenderer renderer(100, 45); // layar lebih lebar
    
    Mat4 proj = Mat4::Perspective(60.0f * M_PI / 180.0f, 100.0f/45.0f, 0.5f, 100.0f);
    Mat4 view = Mat4::LookAt({8,5,15}, {0,1,0}, {0,1,0});
    renderer.SetTransform(proj * view);
    
    std::vector<Vertex> verts;
    std::vector<unsigned> indices;
    
    // Aspal (grid)
    for(int x=-10; x<=10; x+=2) {
        verts.push_back({{(float)x, -0.5f, -10}});
        verts.push_back({{(float)x, -0.5f, 10}});
        indices.push_back(verts.size()-2);
        indices.push_back(verts.size()-1);
    }
    for(int z=-10; z<=10; z+=2) {
        verts.push_back({{-10, -0.5f, (float)z}});
        verts.push_back({{10, -0.5f, (float)z}});
        indices.push_back(verts.size()-2);
        indices.push_back(verts.size()-1);
    }
    
    // Lamborghini (bodi, kabin, roda)
    // Bodi bawah
    AddBox(verts, indices, 0, 1.0f, 0, 4.0f, 0.8f, 2.0f);
    // Kabin
    AddBox(verts, indices, -0.3f, 1.6f, 0, 2.0f, 0.6f, 1.4f);
    // Roda (kotak kecil)
    AddBox(verts, indices, -1.2f, 0.3f, 1.0f, 0.5f, 0.5f, 0.5f);
    AddBox(verts, indices, 1.2f, 0.3f, 1.0f, 0.5f, 0.5f, 0.5f);
    AddBox(verts, indices, -1.2f, 0.3f, -1.0f, 0.5f, 0.5f, 0.5f);
    AddBox(verts, indices, 1.2f, 0.3f, -1.0f, 0.5f, 0.5f, 0.5f);
    
    // Karakter (badan, kepala)
    AddBox(verts, indices, 3.0f, 0.8f, 0, 0.5f, 1.0f, 0.5f);  // badan
    AddBox(verts, indices, 3.0f, 1.6f, 0, 0.4f, 0.4f, 0.4f);  // kepala
    
    renderer.Clear();
    renderer.DrawMesh(verts, indices);
    renderer.Present();
    
    return 0;
}
