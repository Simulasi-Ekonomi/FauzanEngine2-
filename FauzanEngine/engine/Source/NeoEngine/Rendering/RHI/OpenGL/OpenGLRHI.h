#pragma once
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <cstdint>
#include <memory>

namespace NeoEngine {

struct Vertex {
    float position[3];
    float normal[3];
    float texcoord[2];
    float tangent[3];
    float bitangent[3];
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint indexCount = 0;
    bool uploaded = false;

    void Upload();
    void Draw() const;
    void Release();
};

struct TextureData {
    GLuint id = 0;
    int width = 0;
    int height = 0;
    int channels = 0;
    std::string path;

    void Upload(const uint8_t* data, int w, int h, int ch, bool generateMips = true);
    void Bind(GLuint unit = 0) const;
    void Release();
};

struct ShaderUniform {
    enum class Type { Float, Vec2, Vec3, Vec4, Mat4, Int, Sampler2D };
    Type type;
    std::string name;
    int location = -1;
    union { float f; int i; float v2[2]; float v3[3]; float v4[4]; };
};

class Shader {
public:
    Shader() = default;
    ~Shader();

    bool Compile(const char* vertexSrc, const char* fragmentSrc);
    void Bind() const;
    void Unbind() const;

    void SetFloat(const char* name, float value);
    void SetInt(const char* name, int value);
    void SetVec2(const char* name, float x, float y);
    void SetVec3(const char* name, float x, float y, float z);
    void SetVec4(const char* name, float x, float y, float z, float w);
    void SetMat4(const char* name, const float* matrix);
    void SetSampler(const char* name, int textureUnit);

    GLuint GetID() const { return programID; }
    bool IsValid() const { return programID != 0; }

    static bool CheckCompileErrors(GLuint shader, const std::string& type);

private:
    GLuint programID = 0;
    std::unordered_map<std::string, GLint> uniformCache;
    GLint GetUniformLocation(const std::string& name);
};

// Camera for rendering
struct Camera {
    float position[3] = { 0.0f, 2.0f, 8.0f };
    float target[3] = { 0.0f, 0.0f, 0.0f };
    float up[3] = { 0.0f, 1.0f, 0.0f };
    float fov = 60.0f;
    float nearClip = 0.1f;
    float farClip = 1000.0f;
    float aspectRatio = 16.0f / 9.0f;

    void GetViewMatrix(float* out) const;
    void GetProjectionMatrix(float* out) const;
    void GetVPMatrix(float* out) const;
};

// Light structures
struct DirectionalLight {
    float direction[3] = { -0.5f, -1.0f, -0.3f };
    float color[3] = { 1.0f, 0.98f, 0.95f };
    float intensity = 1.0f;
    bool castShadows = true;
};

struct PointLight {
    float position[3] = { 0.0f, 3.0f, 0.0f };
    float color[3] = { 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f;
    float range = 20.0f;
};

// OpenGL RHI - Hardware abstraction for OpenGL ES 3.0
class OpenGLRHI {
public:
    static OpenGLRHI& Get();

    bool Initialize(EGLNativeWindowType window);
    void Shutdown();
    void Resize(int width, int height);

    void BeginFrame();
    void EndFrame();
    void Present();

    void Clear(float r = 0.1f, float g = 0.1f, float b = 0.15f, float a = 1.0f);
    void SetViewport(int x, int y, int w, int h);
    void SetDepthTest(bool enable);
    void SetBlend(bool enable);
    void SetCullFace(bool enable);
    void SetWireframe(bool enable);

    // Mesh management
    MeshData* CreateMesh(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices);
    void DestroyMesh(MeshData* mesh);

    // Texture management
    TextureData* CreateTexture(const uint8_t* data, int w, int h, int ch, bool genMips = true);
    TextureData* LoadTexture(const std::string& path);
    void DestroyTexture(TextureData* tex);

    // Shader management
    Shader* CreateShader(const char* vertexSrc, const char* fragmentSrc);
    Shader* LoadShader(const std::string& name);
    void DestroyShader(Shader* shader);

    // Frame buffer objects
    GLuint CreateFramebuffer(int width, int height, GLuint* colorAttachment, GLuint* depthAttachment);
    void BindFramebuffer(GLuint fbo);
    void BindDefaultFramebuffer();

    // Rendering
    void DrawMesh(MeshData* mesh, Shader* shader);
    void DrawMeshInstanced(MeshData* mesh, Shader* shader, int count);
    void DrawFullscreenQuad();

    // Shadow mapping
    void BeginShadowPass();
    void EndShadowPass();
    GLuint GetShadowMap() const { return shadowMapFBO; }
    GLuint GetShadowMapDepth() const { return shadowMapDepth; }
    float* GetLightSpaceMatrix() { return lightSpaceMatrix; }

    // Queries
    int GetScreenWidth() const { return screenWidth; }
    int GetScreenHeight() const { return screenHeight; }
    bool IsInitialized() const { return initialized; }

    // GPU info
    std::string GetRendererString() const;
    std::string GetVersionString() const;

private:
    OpenGLRHI() = default;
    ~OpenGLRHI();

    bool initialized = false;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;

    int screenWidth = 0;
    int screenHeight = 0;

    // Shadow mapping
    GLuint shadowMapFBO = 0;
    GLuint shadowMapDepth = 0;
    int shadowMapSize = 2048;
    float lightSpaceMatrix[16];

    // Built-in fullscreen quad
    MeshData* fullscreenQuad = nullptr;

    // Resource pools
    std::vector<std::unique_ptr<MeshData>> meshPool;
    std::vector<std::unique_ptr<TextureData>> texturePool;
    std::vector<std::unique_ptr<Shader>> shaderPool;
};

} // namespace NeoEngine
