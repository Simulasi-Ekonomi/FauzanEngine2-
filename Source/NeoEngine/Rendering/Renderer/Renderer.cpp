#include "Renderer.h"
#include "../RHI/OpenGL/OpenGLRHI.h"
#include <GLES3/gl3.h>
#if defined(__ANDROID__)
#include <android/log.h>
#endif
#include <cstdio>
#include <limits>

#if defined(__ANDROID__)
#define LOG_TAG_RENDER "Renderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_RENDER, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_RENDER, __VA_ARGS__)
#else
#define LOGI(...) std::fprintf(stderr, __VA_ARGS__)
#define LOGE(...) std::fprintf(stderr, __VA_ARGS__)
#endif

namespace NeoEngine {

namespace {
bool CompileShader(GLenum type, const char* source, GLuint& shader) {
    shader = 0;
    if (source == nullptr || *source == '\0') return false;
    shader = glCreateShader(type);
    if (shader == 0) return false;
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_TRUE) return true;
    glDeleteShader(shader);
    shader = 0;
    return false;
}

bool LinkProgram(GLuint vertexShader, GLuint fragmentShader, GLuint& program) {
    program = glCreateProgram();
    if (program == 0) return false;
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_TRUE) return true;
    glDeleteProgram(program);
    program = 0;
    return false;
}
}

Renderer& Renderer::Get() {
    static Renderer instance;
    return instance;
}

bool Renderer::Init(void* window, int w, int h) {
    if (m_Initialized) return w == m_Width && h == m_Height;
    if (window == nullptr || w <= 0 || h <= 0) return false;
    OpenGLRHI& rhi = OpenGLRHI::Get();
    if (!rhi.Initialize(static_cast<EGLNativeWindowType>(window))) {
        LOGE("Failed to initialize OpenGL RHI\n");
        return false;
    }
    m_Width = w;
    m_Height = h;
    rhi.SetViewport(0, 0, w, h);
    m_Initialized = true;
    LOGI("Renderer initialized: %dx%d\n", w, h);
    return true;
}

void Renderer::Shutdown() {
    for (auto& mesh : m_Meshes) {
        if (mesh && mesh->vao) glDeleteVertexArrays(1, &mesh->vao);
        if (mesh && mesh->vbo) glDeleteBuffers(1, &mesh->vbo);
        if (mesh && mesh->ebo) glDeleteBuffers(1, &mesh->ebo);
    }
    for (auto& texture : m_Textures) {
        if (texture && texture->id) glDeleteTextures(1, &texture->id);
    }
    for (auto& shader : m_Shaders) {
        if (shader && shader->program) glDeleteProgram(shader->program);
    }
    m_Meshes.clear();
    m_Textures.clear();
    m_Shaders.clear();
    OpenGLRHI::Get().Shutdown();
    m_Width = 0;
    m_Height = 0;
    m_Initialized = false;
    LOGI("Renderer shutdown\n");
}

void Renderer::BeginFrame() {
    if (m_Initialized) OpenGLRHI::Get().BeginFrame();
}

void Renderer::EndFrame() {
    if (m_Initialized) OpenGLRHI::Get().EndFrame();
}

void Renderer::Clear(float r, float g, float b, float a) {
    if (m_Initialized) OpenGLRHI::Get().Clear(r, g, b, a);
}

Mesh* Renderer::CreateMesh(const float* vertices, int vcount, const unsigned int* indices, int icount) {
    if (!m_Initialized || !vertices || !indices || vcount <= 0 || icount <= 0) return nullptr;
    if (static_cast<unsigned long long>(vcount) > std::numeric_limits<GLsizei>::max() ||
        static_cast<unsigned long long>(icount) > std::numeric_limits<GLsizei>::max()) return nullptr;

    auto mesh = std::make_unique<Mesh>();
    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo);
    glGenBuffers(1, &mesh->ebo);
    if (!mesh->vao || !mesh->vbo || !mesh->ebo) {
        if (mesh->vao) glDeleteVertexArrays(1, &mesh->vao);
        if (mesh->vbo) glDeleteBuffers(1, &mesh->vbo);
        if (mesh->ebo) glDeleteBuffers(1, &mesh->ebo);
        return nullptr;
    }

    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vcount) * 3 * sizeof(float), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(icount) * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    if (glGetError() != GL_NO_ERROR) {
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &mesh->vao);
        glDeleteBuffers(1, &mesh->vbo);
        glDeleteBuffers(1, &mesh->ebo);
        return nullptr;
    }
    glBindVertexArray(0);
    mesh->indexCount = icount;
    m_Meshes.push_back(std::move(mesh));
    return m_Meshes.back().get();
}

Texture* Renderer::CreateTexture(const unsigned char* data, int w, int h, int channels) {
    if (!m_Initialized || !data || w <= 0 || h <= 0 || channels < 1 || channels > 4) return nullptr;
    GLenum format = channels == 1 ? GL_RED : channels == 2 ? GL_RG : channels == 3 ? GL_RGB : GL_RGBA;
    auto texture = std::make_unique<Texture>();
    glGenTextures(1, &texture->id);
    if (!texture->id) return nullptr;
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), w, h, 0, format, GL_UNSIGNED_BYTE, data);
    if (glGetError() != GL_NO_ERROR) {
        glDeleteTextures(1, &texture->id);
        return nullptr;
    }
    texture->width = w;
    texture->height = h;
    m_Textures.push_back(std::move(texture));
    return m_Textures.back().get();
}

Shader* Renderer::CreateShader(const char* vsSource, const char* fsSource) {
    if (!m_Initialized || !vsSource || !fsSource) return nullptr;
    GLuint vertexShader = 0, fragmentShader = 0;
    if (!CompileShader(GL_VERTEX_SHADER, vsSource, vertexShader) ||
        !CompileShader(GL_FRAGMENT_SHADER, fsSource, fragmentShader)) {
        if (vertexShader) glDeleteShader(vertexShader);
        if (fragmentShader) glDeleteShader(fragmentShader);
        return nullptr;
    }
    GLuint program = 0;
    const bool linked = LinkProgram(vertexShader, fragmentShader, program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!linked) return nullptr;
    auto shader = std::make_unique<Shader>();
    shader->program = program;
    m_Shaders.push_back(std::move(shader));
    return m_Shaders.back().get();
}

void Renderer::DrawMesh(Mesh* mesh, Shader* shader) {
    if (!m_Initialized || !mesh || !shader || !mesh->vao || !mesh->ebo || !shader->program || mesh->indexCount <= 0) return;
    glUseProgram(shader->program);
    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Renderer::SetViewport(int x, int y, int w, int h) {
    if (!m_Initialized || w <= 0 || h <= 0) return;
    m_Width = w;
    m_Height = h;
    OpenGLRHI::Get().SetViewport(x, y, w, h);
}

}
