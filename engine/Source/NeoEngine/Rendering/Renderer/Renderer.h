#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
namespace NeoEngine {
struct Mesh{unsigned int vao=0,vbo=0,ebo=0;int indexCount=0;};
struct Texture{unsigned int id=0;int w=0,h=0;};
struct Shader{unsigned int program=0;};
class Renderer {
public:
    static Renderer& Get();
    bool Init(void* window,int w,int h); void Shutdown();
    void BeginFrame(); void EndFrame(); void Clear(float r,float g,float b);
    Mesh* CreateMesh(const float* verts,int vcount,const unsigned int* indices,int icount);
    Texture* CreateTexture(const unsigned char* data,int w,int h,int channels);
    Shader* CreateShader(const char* vs,const char* fs);
    void DrawMesh(Mesh* m,Shader* s); void SetViewport(int x,int y,int w,int h);
private:
    bool m_Init=false; std::vector<std::unique_ptr<Mesh>> m_Meshes;
    std::vector<std::unique_ptr<Texture>> m_Textures; std::vector<std::unique_ptr<Shader>> m_Shaders;
};
}
