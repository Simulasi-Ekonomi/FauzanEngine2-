#include "OpenGLRHI.h"
#include <android/log.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cmath>

#define LOG_TAG "OpenGLRHI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

OpenGLRHI& OpenGLRHI::Get() {
    static OpenGLRHI instance;
    return instance;
}

bool OpenGLRHI::Initialize(EGLNativeWindowType window) {
    if (m_Initialized) return true;
    m_Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(m_Display, nullptr, nullptr)) {
        LOGE("Failed to initialize EGL");
        return false;
    }
    EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(m_Display, attribs, &config, 1, &numConfigs);
    m_Surface = eglCreateWindowSurface(m_Display, config, window, nullptr);
    EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    m_Context = eglCreateContext(m_Display, config, EGL_NO_CONTEXT, ctxAttribs);
    eglMakeCurrent(m_Display, m_Surface, m_Surface, m_Context);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    m_Initialized = true;
    LOGI("OpenGLRHI initialized: %s", glGetString(GL_RENDERER));
    return true;
}

void OpenGLRHI::Shutdown() {
    if (m_Context != EGL_NO_CONTEXT) eglDestroyContext(m_Display, m_Context);
    if (m_Surface != EGL_NO_SURFACE) eglDestroySurface(m_Display, m_Surface);
    if (m_Display != EGL_NO_DISPLAY) eglTerminate(m_Display);
    m_Initialized = false;
}

void OpenGLRHI::BeginFrame() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
void OpenGLRHI::EndFrame() { eglSwapBuffers(m_Display, m_Surface); }

void OpenGLRHI::Clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRHI::SetViewport(int x, int y, int w, int h) { glViewport(x, y, w, h); }
void OpenGLRHI::SetDepthTest(bool enable) { enable ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST); }
void OpenGLRHI::SetBlend(bool enable) { enable ? glEnable(GL_BLEND) : glDisable(GL_BLEND); }
void OpenGLRHI::SetCullFace(bool enable) { enable ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE); }
void OpenGLRHI::SetWireframe(bool enable) { glPolygonMode(GL_FRONT_AND_BACK, enable ? GL_LINE : GL_FILL); }

std::string OpenGLRHI::GetRendererString() const {
    return m_Initialized ? reinterpret_cast<const char*>(glGetString(GL_RENDERER)) : "";
}

std::string OpenGLRHI::GetVersionString() const {
    return m_Initialized ? reinterpret_cast<const char*>(glGetString(GL_VERSION)) : "";
}

} // namespace NeoEngine
