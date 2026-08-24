#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../RHI/RHI.h"

namespace NeoEngine {

// Tipe resource rendering (setara UStaticMesh, UTexture2D, UShader)
struct Mesh {
    unsigned int vao = 0, vbo = 0, ebo = 0;
    int indexCount = 0;
};

struct Texture {
    unsigned int id = 0;
    int width = 0, height = 0;
};

struct Shader {
    unsigned int program = 0;
};

class Renderer {
public:
    static Renderer& Get();
    
    bool Init(void* window, int w, int h);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    void Clear(float r = 0.1f, float g = 0.1f, float b = 0.15f, float a = 1.0f);
    
    Mesh* CreateMesh(const float* vertices, int vcount, const unsigned int* indices, int icount);
    Texture* CreateTexture(const unsigned char* data, int w, int h, int channels);
    Shader* CreateShader(const char* vsSource, const char* fsSource);
    
    void DrawMesh(Mesh* mesh, Shader* shader);
    void SetViewport(int x, int y, int w, int h);
    
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    bool IsInitialized() const { return m_Initialized; }

private:
    Renderer() = default;
    
    bool m_Initialized = false;
    int m_Width = 0, m_Height = 0;
    std::vector<std::unique_ptr<Mesh>> m_Meshes;
    std::vector<std::unique_ptr<Texture>> m_Textures;
    std::vector<std::unique_ptr<Shader>> m_Shaders;
};

} // namespace NeoEngine
