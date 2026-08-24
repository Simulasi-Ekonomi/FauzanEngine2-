#pragma once
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdio>

namespace NeoEngine {

class GPURenderer {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;
    GLuint program = 0, vbo = 0, vao = 0;

public:
    bool Init(int w = 1280, int h = 720) {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglInitialize(display, nullptr, nullptr);
        const EGLint att[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_NONE};
        EGLConfig cfg; EGLint n;
        eglChooseConfig(display, att, &cfg, 1, &n);
        const EGLint ctxAtt[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        context = eglCreateContext(display, cfg, EGL_NO_CONTEXT, ctxAtt);
        const EGLint pb[] = {EGL_WIDTH, w, EGL_HEIGHT, h, EGL_NONE};
        surface = eglCreatePbufferSurface(display, cfg, pb);
        eglMakeCurrent(display, surface, surface, context);

        // Shader
        const char* vs = "#version 300 es\n"
            "in vec3 aPos; void main() { gl_Position = vec4(aPos, 1.0); }";
        const char* fs = "#version 300 es\n"
            "precision highp float; out vec4 c; void main() { c = vec4(1,0,0,1); }";
        GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &vs, 0); glCompileShader(v);
        GLuint f = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(f, 1, &fs, 0); glCompileShader(f);
        program = glCreateProgram(); glAttachShader(program, v); glAttachShader(program, f); glLinkProgram(program);

        // Geometry
        float tri[] = { 0,0.5f,0, -0.5f,-0.5f,0, 0.5f,-0.5f,0 };
        glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
        glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(tri), tri, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        return true;
    }

    void Render() {
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glFinish();
    }

    void Shutdown() {
        eglDestroyContext(display, context);
        eglDestroySurface(display, surface);
        eglTerminate(display);
    }
};

} // namespace NeoEngine
