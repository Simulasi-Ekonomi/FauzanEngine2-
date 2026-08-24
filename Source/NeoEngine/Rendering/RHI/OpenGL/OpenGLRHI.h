#pragma once
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <string>

namespace NeoEngine {

class OpenGLRHI {
public:
    static OpenGLRHI& Get();
    
    bool Initialize(EGLNativeWindowType window);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    void Clear(float r, float g, float b, float a);
    void SetViewport(int x, int y, int w, int h);
    
    void SetDepthTest(bool enable);
    void SetBlend(bool enable);
    void SetCullFace(bool enable);
    void SetWireframe(bool enable);
    
    std::string GetRendererString() const;
    std::string GetVersionString() const;
    
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    bool IsInitialized() const { return m_Initialized; }

private:
    OpenGLRHI() = default;
    
    EGLDisplay m_Display = EGL_NO_DISPLAY;
    EGLSurface m_Surface = EGL_NO_SURFACE;
    EGLContext m_Context = EGL_NO_CONTEXT;
    int m_Width = 0, m_Height = 0;
    bool m_Initialized = false;
};

} // namespace NeoEngine
