#include "OpenGLRHI.h"
#if defined(__ANDROID__)
#include <android/log.h>
#endif
#include <cstring>
#include <fstream>
#include <sstream>
#include <cmath>

#if defined(__ANDROID__)
#define LOG_TAG_GL "OpenGLRHI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_GL, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_GL, __VA_ARGS__)
#else
#include <cstdio>
#define LOGI(...) std::fprintf(stderr, __VA_ARGS__)
#define LOGE(...) std::fprintf(stderr, __VA_ARGS__)
#endif

namespace NeoEngine {

OpenGLRHI& OpenGLRHI::Get() {
    static OpenGLRHI instance;
    return instance;
}

bool OpenGLRHI::Initialize(EGLNativeWindowType window) {
    if (window == EGLNativeWindowType{}) return false;
    if (m_Initialized) return !m_Headless && m_Window == window;
    return InitializeSurface(EGL_WINDOW_BIT, window, 0, 0);
}

bool OpenGLRHI::InitializeHeadless(int width, int height) {
    if (width <= 0 || height <= 0) return false;
    if (m_Initialized) return m_Headless && m_Width == width && m_Height == height;
    return InitializeSurface(EGL_PBUFFER_BIT, EGLNativeWindowType{}, width, height);
}

bool OpenGLRHI::InitializeSurface(EGLint surfaceType, EGLNativeWindowType window, int width, int height) {

    m_Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(m_Display, nullptr, nullptr)) {
        LOGE("Failed to initialize EGL (error=0x%x)\n", eglGetError());
        m_Display = EGL_NO_DISPLAY;
        return false;
    }
    m_EglInitialized = true;

    EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, surfaceType,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };

    EGLConfig config = nullptr;
    EGLint numConfigs = 0;
    if (eglChooseConfig(m_Display, attribs, &config, 1, &numConfigs) != EGL_TRUE || numConfigs < 1) {
        LOGE("No compatible EGL config (error=0x%x)\n", eglGetError());
        Shutdown();
        return false;
    }

    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
        LOGE("Failed to bind OpenGL ES API (error=0x%x)\n", eglGetError());
        Shutdown();
        return false;
    }

    if (surfaceType == EGL_PBUFFER_BIT) {
        const EGLint surfaceAttribs[] = {EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE};
        m_Surface = eglCreatePbufferSurface(m_Display, config, surfaceAttribs);
    } else {
        m_Surface = eglCreateWindowSurface(m_Display, config, window, nullptr);
    }
    if (m_Surface == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL surface (error=0x%x)\n", eglGetError());
        Shutdown();
        return false;
    }

    EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    m_Context = eglCreateContext(m_Display, config, EGL_NO_CONTEXT, ctxAttribs);
    if (m_Context == EGL_NO_CONTEXT) {
        LOGE("Failed to create OpenGL ES context (error=0x%x)\n", eglGetError());
        Shutdown();
        return false;
    }

    if (eglMakeCurrent(m_Display, m_Surface, m_Surface, m_Context) != EGL_TRUE) {
        LOGE("Failed to make EGL context current (error=0x%x)\n", eglGetError());
        Shutdown();
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    m_Initialized = true;
    m_Headless = surfaceType == EGL_PBUFFER_BIT;
    m_Window = m_Headless ? EGLNativeWindowType{} : window;
    m_Width = width;
    m_Height = height;
    const auto* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    LOGI("OpenGLRHI initialized: %s\n", renderer ? renderer : "unknown renderer");
    return true;
}

void OpenGLRHI::Shutdown() {
    if (m_Display != EGL_NO_DISPLAY) {
        if (m_EglInitialized) eglMakeCurrent(m_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (m_Context != EGL_NO_CONTEXT) eglDestroyContext(m_Display, m_Context);
        if (m_Surface != EGL_NO_SURFACE) eglDestroySurface(m_Display, m_Surface);
        if (m_EglInitialized) eglTerminate(m_Display);
    }
    m_Display = EGL_NO_DISPLAY;
    m_Surface = EGL_NO_SURFACE;
    m_Context = EGL_NO_CONTEXT;
    m_EglInitialized = false;
    m_Initialized = false;
    m_Headless = false;
    m_Window = EGLNativeWindowType{};
    m_Width = 0;
    m_Height = 0;
}

void OpenGLRHI::BeginFrame() { if (m_Initialized) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
void OpenGLRHI::EndFrame() {
    if (m_Initialized && eglSwapBuffers(m_Display, m_Surface) != EGL_TRUE)
        LOGE("Failed to swap EGL buffers (error=0x%x)\n", eglGetError());
}

void OpenGLRHI::Clear(float r, float g, float b, float a) {
    if (!m_Initialized) return;
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRHI::SetViewport(int x, int y, int w, int h) {
    if (m_Initialized && w > 0 && h > 0) {
        glViewport(x, y, w, h);
        m_Width = w;
        m_Height = h;
    }
}

void OpenGLRHI::SetDepthTest(bool enable) { if (m_Initialized) enable ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST); }
void OpenGLRHI::SetBlend(bool enable) { if (m_Initialized) enable ? glEnable(GL_BLEND) : glDisable(GL_BLEND); }
void OpenGLRHI::SetCullFace(bool enable) { if (m_Initialized) enable ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE); }

// SetWireframe tidak didukung di OpenGL ES 3.0 (hanya desktop)
void OpenGLRHI::SetWireframe(bool enable) {
    // glPolygonMode tidak tersedia di GLES. Fitur wireframe hanya untuk debugging desktop.
    (void)enable; // suppress unused warning
}

std::string OpenGLRHI::GetRendererString() const {
    if (!m_Initialized) return {};
    const auto* value = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    return value ? value : "";
}

std::string OpenGLRHI::GetVersionString() const {
    if (!m_Initialized) return {};
    const auto* value = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    return value ? value : "";
}

} // namespace NeoEngine
