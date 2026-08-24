#include "OpenGLRHI.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "NeoEngine_RHI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

// ======================== Math Helpers ========================

static void Mat4Identity(float* m) {
    m[0]=1; m[1]=0; m[2]=0;  m[3]=0;
    m[4]=0; m[5]=1; m[6]=0;  m[7]=0;
    m[8]=0; m[9]=0; m[10]=1; m[11]=0;
    m[12]=0;m[13]=0;m[14]=0; m[15]=1;
}

static void Mat4Multiply(float* out, const float* a, const float* b) {
    float tmp[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            tmp[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++) {
                tmp[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
    memcpy(out, tmp, sizeof(tmp));
}

static void Mat4Perspective(float* out, float fovDeg, float aspect, float nearP, float farP) {
    float fovRad = fovDeg * 3.14159265f / 180.0f;
    float f = 1.0f / tanf(fovRad / 2.0f);
    Mat4Identity(out);
    out[0] = f / aspect;
    out[5] = f;
    out[10] = (farP + nearP) / (nearP - farP);
    out[11] = -1.0f;
    out[14] = (2.0f * farP * nearP) / (nearP - farP);
    out[15] = 0.0f;
}

static void Mat4LookAt(float* out, const float* eye, const float* center, const float* up) {
    float f[3] = { center[0]-eye[0], center[1]-eye[1], center[2]-eye[2] };
    float len = sqrtf(f[0]*f[0]+f[1]*f[1]+f[2]*f[2]);
    f[0]/=len; f[1]/=len; f[2]/=len;

    float s[3] = { f[1]*up[2]-f[2]*up[1], f[2]*up[0]-f[0]*up[2], f[0]*up[1]-f[1]*up[0] };
    len = sqrtf(s[0]*s[0]+s[1]*s[1]+s[2]*s[2]);
    s[0]/=len; s[1]/=len; s[2]/=len;

    float u[3] = { s[1]*f[2]-s[2]*f[1], s[2]*f[0]-s[0]*f[2], s[0]*f[1]-s[1]*f[0] };

    Mat4Identity(out);
    out[0]=s[0];  out[4]=s[1];  out[8]=s[2];   out[12]=-(s[0]*eye[0]+s[1]*eye[1]+s[2]*eye[2]);
    out[1]=u[0];  out[5]=u[1];  out[9]=u[2];   out[13]=-(u[0]*eye[0]+u[1]*eye[1]+u[2]*eye[2]);
    out[2]=-f[0]; out[6]=-f[1]; out[10]=-f[2]; out[14]=(f[0]*eye[0]+f[1]*eye[1]+f[2]*eye[2]);
    out[3]=0;     out[7]=0;     out[11]=0;     out[15]=1;
}

static void Mat4Ortho(float* out, float left, float right, float bottom, float top, float nearP, float farP) {
    Mat4Identity(out);
    out[0] = 2.0f / (right - left);
    out[5] = 2.0f / (top - bottom);
    out[10] = -2.0f / (farP - nearP);
    out[12] = -(right + left) / (right - left);
    out[13] = -(top + bottom) / (top - bottom);
    out[14] = -(farP + nearP) / (farP - nearP);
}

static void Mat4Translate(float* out, float x, float y, float z) {
    Mat4Identity(out);
    out[12] = x; out[13] = y; out[14] = z;
}

// ======================== Camera ========================

void Camera::GetViewMatrix(float* out) const {
    Mat4LookAt(out, position, target, up);
}

void Camera::GetProjectionMatrix(float* out) const {
    Mat4Perspective(out, fov, aspectRatio, nearClip, farClip);
}

void Camera::GetVPMatrix(float* out) const {
    float view[16], proj[16];
    GetViewMatrix(view);
    GetProjectionMatrix(proj);
    Mat4Multiply(out, proj, view);
}

// ======================== MeshData ========================

void MeshData::Upload() {
    if (uploaded) return;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // Texcoord
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));
    // Tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    // Bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

    glBindVertexArray(0);
    indexCount = static_cast<GLuint>(indices.size());
    uploaded = true;
}

void MeshData::Draw() const {
    if (!uploaded) return;
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void MeshData::Release() {
    if (!uploaded) return;
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    vao = vbo = ebo = 0;
    indexCount = 0;
    uploaded = false;
}

// ======================== TextureData ========================

void TextureData::Upload(const uint8_t* data, int w, int h, int ch, bool generateMips) {
    width = w; height = h; channels = ch;

    GLenum format = GL_RGBA;
    GLenum internalFormat = GL_RGBA;
    if (ch == 3) { format = GL_RGB; internalFormat = GL_RGB; }
    else if (ch == 1) { format = GL_RED; internalFormat = GL_R8; }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, GL_UNSIGNED_BYTE, data);

    if (generateMips) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, generateMips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextureData::Bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

void TextureData::Release() {
    if (id) { glDeleteTextures(1, &id); id = 0; }
}

// ======================== Shader ========================

Shader::~Shader() {
    if (programID) { glDeleteProgram(programID); programID = 0; }
}

bool Shader::CheckCompileErrors(GLuint shader, const std::string& type) {
    GLint success;
    char infoLog[512];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            LOGE("Shader compile error (%s): %s", type.c_str(), infoLog);
            return false;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 512, nullptr, infoLog);
            LOGE("Shader link error: %s", infoLog);
            return false;
        }
    }
    return true;
}

bool Shader::Compile(const char* vertexSrc, const char* fragmentSrc) {
    if (programID) { glDeleteProgram(programID); programID = 0; }

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexSrc, nullptr);
    glCompileShader(vs);
    if (!CheckCompileErrors(vs, "VERTEX")) { glDeleteShader(vs); return false; }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentSrc, nullptr);
    glCompileShader(fs);
    if (!CheckCompileErrors(fs, "FRAGMENT")) { glDeleteShader(vs); glDeleteShader(fs); return false; }

    programID = glCreateProgram();
    glAttachShader(programID, vs);
    glAttachShader(programID, fs);
    glLinkProgram(programID);
    if (!CheckCompileErrors(programID, "PROGRAM")) {
        glDeleteShader(vs); glDeleteShader(fs); glDeleteProgram(programID);
        programID = 0; return false;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    uniformCache.clear();
    return true;
}

void Shader::Bind() const { if (programID) glUseProgram(programID); }
void Shader::Unbind() const { glUseProgram(0); }

GLint Shader::GetUniformLocation(const std::string& name) {
    auto it = uniformCache.find(name);
    if (it != uniformCache.end()) return it->second;
    GLint loc = glGetUniformLocation(programID, name.c_str());
    uniformCache[name] = loc;
    return loc;
}

void Shader::SetFloat(const char* name, float value) {
    GLint loc = GetUniformLocation(name);
    if (loc >= 0) glUniform1f(loc, value);
}
void Shader::SetInt(const char* name, int value) {
    GLint loc = GetUniformLocation(name);
    if (loc >= 0) glUniform1i(loc, value);
}
void Shader::SetVec2(const char* name, float x, float y) {
    GLint loc = GetUniformLocation(name);
    if (loc >= 0) glUniform2f(loc, x, y);
}
void Shader::SetVec3(const char* name, float x, float y, float z) {
    GLint loc = GetUniformLocation(name);
    if (loc >= 0) glUniform3f(loc, x, y, z);
}
void Shader::SetVec4(const char* name, float x, float y, float z, float w) {
    GLint loc = GetUniformLocation(name);
    if (loc >= 0) glUniform4f(loc, x, y, z, w);
}
void Shader::SetMat4(const char* name, const float* matrix) {
    GLint loc = GetUniformLocation(name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, matrix);
}
void Shader::SetSampler(const char* name, int textureUnit) {
    GLint loc = GetUniformLocation(name);
    if (loc >= 0) glUniform1i(loc, textureUnit);
}

// ======================== OpenGLRHI ========================

OpenGLRHI& OpenGLRHI::Get() {
    static OpenGLRHI instance;
    return instance;
}

bool OpenGLRHI::Initialize(EGLNativeWindowType window) {
    if (initialized) return true;

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        LOGE("Failed to get EGL display");
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(display, &major, &minor)) {
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
    if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs) || numConfigs == 0) {
        LOGE("Failed to choose EGL config");
        return false;
    }

    EGLint surfaceAttribs[] = { EGL_NONE };
    surface = eglCreateWindowSurface(display, config, window, surfaceAttribs);
    if (surface == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL surface");
        return false;
    }

    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        LOGE("Failed to create EGL context");
        return false;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        LOGE("Failed to make EGL context current");
        return false;
    }

    // Log GPU info
    LOGI("Renderer: %s", glGetString(GL_RENDERER));
    LOGI("Version: %s", glGetString(GL_VERSION));

    // Setup default state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    // Init shadow map
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(1, &shadowMapDepth);
    glBindTexture(GL_TEXTURE_2D, shadowMapDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapDepth, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Init fullscreen quad
    {
        fullscreenQuad = new MeshData();
        fullscreenQuad->vertices = {
            {{-1,-1,0},{0,0,1},{0,0},{1,0,0},{0,1,0}},
            {{ 1,-1,0},{0,0,1},{1,0},{1,0,0},{0,1,0}},
            {{ 1, 1,0},{0,0,1},{1,1},{1,0,0},{0,1,0}},
            {{-1, 1,0},{0,0,1},{0,1},{1,0,0},{0,1,0}},
        };
        fullscreenQuad->indices = {0,1,2, 0,2,3};
        fullscreenQuad->Upload();
    }

    initialized = true;
    LOGI("OpenGLRHI initialized successfully (GLES 3.0)");
    return true;
}

void OpenGLRHI::Shutdown() {
    if (!initialized) return;

    if (fullscreenQuad) { fullscreenQuad->Release(); delete fullscreenQuad; fullscreenQuad = nullptr; }

    if (shadowMapDepth) glDeleteTextures(1, &shadowMapDepth);
    if (shadowMapFBO) glDeleteFramebuffers(1, &shadowMapFBO);

    for (auto& mesh : meshPool) mesh->Release();
    for (auto& tex : texturePool) tex->Release();
    meshPool.clear();
    texturePool.clear();
    shaderPool.clear();

    if (context != EGL_NO_CONTEXT) {
        eglDestroyContext(display, context);
        context = EGL_NO_CONTEXT;
    }
    if (surface != EGL_NO_SURFACE) {
        eglDestroySurface(display, surface);
        surface = EGL_NO_SURFACE;
    }
    if (display != EGL_NO_DISPLAY) {
        eglTerminate(display);
        display = EGL_NO_DISPLAY;
    }

    initialized = false;
}

void OpenGLRHI::Resize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    if (display && surface) {
        eglQuerySurface(display, surface, EGL_WIDTH, &screenWidth);
        eglQuerySurface(display, surface, EGL_HEIGHT, &screenHeight);
    }
    glViewport(0, 0, screenWidth, screenHeight);
}

void OpenGLRHI::BeginFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void OpenGLRHI::EndFrame() {}
void OpenGLRHI::Present() {
    if (display && surface) eglSwapBuffers(display, surface);
}

void OpenGLRHI::Clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void OpenGLRHI::SetViewport(int x, int y, int w, int h) { glViewport(x, y, w, h); }
void OpenGLRHI::SetDepthTest(bool enable) { enable ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST); }
void OpenGLRHI::SetBlend(bool enable) { enable ? glEnable(GL_BLEND) : glDisable(GL_BLEND); }
void OpenGLRHI::SetCullFace(bool enable) { enable ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE); }
void OpenGLRHI::SetWireframe(bool enable) { glPolygonMode(GL_FRONT_AND_BACK, enable ? GL_LINE : GL_FILL); }

MeshData* OpenGLRHI::CreateMesh(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices) {
    auto mesh = std::make_unique<MeshData>();
    mesh->vertices = verts;
    mesh->indices = indices;
    mesh->Upload();
    meshPool.push_back(std::move(mesh));
    return meshPool.back().get();
}
void OpenGLRHI::DestroyMesh(MeshData* mesh) {
    if (!mesh) return;
    mesh->Release();
    meshPool.erase(std::remove_if(meshPool.begin(), meshPool.end(),
        [mesh](const std::unique_ptr<MeshData>& m) { return m.get() == mesh; }), meshPool.end());
}

TextureData* OpenGLRHI::CreateTexture(const uint8_t* data, int w, int h, int ch, bool genMips) {
    auto tex = std::make_unique<TextureData>();
    tex->Upload(data, w, h, ch, genMips);
    texturePool.push_back(std::move(tex));
    return texturePool.back().get();
}

TextureData* OpenGLRHI::LoadTexture(const std::string& path) {
    // TODO: Implement texture loading from file (PNG, JPG, etc.)
    LOGE("LoadTexture not implemented for: %s", path.c_str());
    return nullptr;
}
void OpenGLRHI::DestroyTexture(TextureData* tex) {
    if (!tex) return;
    tex->Release();
    texturePool.erase(std::remove_if(texturePool.begin(), texturePool.end(),
        [tex](const std::unique_ptr<TextureData>& t) { return t.get() == tex; }), texturePool.end());
}

Shader* OpenGLRHI::CreateShader(const char* vs, const char* fs) {
    auto shader = std::make_unique<Shader>();
    shader->Compile(vs, fs);
    shaderPool.push_back(std::move(shader));
    return shaderPool.back().get();
}

Shader* OpenGLRHI::LoadShader(const std::string& name) {
    // TODO: Load shader from file
    return nullptr;
}
void OpenGLRHI::DestroyShader(Shader* shader) {
    if (!shader) return;
    shaderPool.erase(std::remove_if(shaderPool.begin(), shaderPool.end(),
        [shader](const std::unique_ptr<Shader>& s) { return s.get() == shader; }), shaderPool.end());
}

GLuint OpenGLRHI::CreateFramebuffer(int width, int height, GLuint* colorAttachment, GLuint* depthAttachment) {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    if (colorAttachment) {
        glGenTextures(1, colorAttachment);
        glBindTexture(GL_TEXTURE_2D, *colorAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *colorAttachment, 0);
    }

    if (depthAttachment) {
        glGenTextures(1, depthAttachment);
        glBindTexture(GL_TEXTURE_2D, *depthAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *depthAttachment, 0);
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Framebuffer not complete: %d", status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return fbo;
}

void OpenGLRHI::BindFramebuffer(GLuint fbo) { glBindFramebuffer(GL_FRAMEBUFFER, fbo); }
void OpenGLRHI::BindDefaultFramebuffer() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void OpenGLRHI::DrawMesh(MeshData* mesh, Shader* shader) {
    if (!mesh || !shader || !mesh->uploaded) return;
    shader->Bind();
    mesh->Draw();
}

void OpenGLRHI::DrawMeshInstanced(MeshData* mesh, Shader* shader, int count) {
    if (!mesh || !shader || !mesh->uploaded || count <= 0) return;
    shader->Bind();
    glBindVertexArray(mesh->vao);
    glDrawElementsInstanced(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, nullptr, count);
    glBindVertexArray(0);
}

void OpenGLRHI::DrawFullscreenQuad() {
    if (fullscreenQuad) fullscreenQuad->Draw();
}

void OpenGLRHI::BeginShadowPass() {
    glViewport(0, 0, shadowMapSize, shadowMapSize);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Light space matrix (directional light looking down from above)
    float lightView[16], lightProj[16];
    float lightPos[3] = { -5.0f, 10.0f, -5.0f };
    float lightTarget[3] = { 0.0f, 0.0f, 0.0f };
    float lightUp[3] = { 0.0f, 1.0f, 0.0f };
    Mat4LookAt(lightView, lightPos, lightTarget, lightUp);
    Mat4Ortho(lightProj, -20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 50.0f);
    Mat4Multiply(lightSpaceMatrix, lightProj, lightView);
}

void OpenGLRHI::EndShadowPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    SetViewport(0, 0, screenWidth, screenHeight);
}

std::string OpenGLRHI::GetRendererString() const {
    return initialized ? reinterpret_cast<const char*>(glGetString(GL_RENDERER)) : "";
}
std::string OpenGLRHI::GetVersionString() const {
    return initialized ? reinterpret_cast<const char*>(glGetString(GL_VERSION)) : "";
}

} // namespace NeoEngine
