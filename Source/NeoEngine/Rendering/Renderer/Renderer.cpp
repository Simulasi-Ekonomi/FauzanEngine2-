#include "Renderer.h"
#include "../RHI/OpenGL/OpenGLRHI.h"
#include <android/log.h>
#include <cstring>

#define LOG_TAG_RENDER "Renderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_RENDER, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_RENDER, __VA_ARGS__)

namespace NeoEngine {

Renderer& Renderer::Get() {
    static Renderer instance;
    return instance;
}

bool Renderer::Init(void* window, int w, int h) {
    m_Width = w;
    m_Height = h;

    OpenGLRHI& rhi = OpenGLRHI::Get();
    if (!rhi.Initialize((EGLNativeWindowType)window)) {
        LOGE("Failed to initialize RHI");
        return false;
    }

    m_Initialized = true;
    LOGI("Renderer initialized: %dx%d", w, h);
    return true;
}

void Renderer::Shutdown() {
    m_Meshes.clear();
    m_Textures.clear();
    m_Shaders.clear();
    m_Initialized = false;
    LOGI("Renderer shutdown");
}

void Renderer::BeginFrame() {
    if (!m_Initialized) return;
    OpenGLRHI::Get().BeginFrame();
}

void Renderer::EndFrame() {
    if (!m_Initialized) return;
    OpenGLRHI::Get().EndFrame();
}

void Renderer::Clear(float r, float g, float b, float a) {
    if (!m_Initialized) return;
    OpenGLRHI::Get().Clear(r, g, b, a);
}

Mesh* Renderer::CreateMesh(const float* vertices, int vcount, const unsigned int* indices, int icount) {
    auto mesh = std::make_unique<Mesh>();
    m_Meshes.push_back(std::move(mesh));
    return m_Meshes.back().get();
}

Texture* Renderer::CreateTexture(const unsigned char* data, int w, int h, int channels) {
    auto tex = std::make_unique<Texture>();
    tex->width = w;
    tex->height = h;
    m_Textures.push_back(std::move(tex));
    return m_Textures.back().get();
}

Shader* Renderer::CreateShader(const char* vsSource, const char* fsSource) {
    auto shader = std::make_unique<Shader>();
    m_Shaders.push_back(std::move(shader));
    return m_Shaders.back().get();
}

void Renderer::DrawMesh(Mesh* mesh, Shader* shader) {
    if (!mesh || !shader) return;
}

void Renderer::SetViewport(int x, int y, int w, int h) {
    m_Width = w;
    m_Height = h;
    OpenGLRHI::Get().SetViewport(x, y, w, h);
}

} // namespace NeoEngine
