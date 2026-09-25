#include "RHI.h"
#include <GLES3/gl31.h>
#if defined(__ANDROID__)
#include <android/log.h>
#endif
#include <cstdio>

#if defined(__ANDROID__)
#define LOG_TAG "NeoRHI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) std::fprintf(stderr, __VA_ARGS__)
#endif

namespace NeoEngine {

RHI& RHI::Get() {
    static RHI instance;
    return instance;
}

bool RHI::Initialize() {
    if (initialized_) return true;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    initialized_ = true;
    LOGI("RHI Initialized - OpenGL ES 3.1\n");
    return true;
}

void RHI::Shutdown() {
    if (!initialized_) return;
    initialized_ = false;
}

RHIBuffer RHI::CreateVertexBuffer(const void* data, size_t size) {
    RHIBuffer buf;
    if (!initialized_ || data == nullptr || size == 0) return buf;

    glGenBuffers(1, &buf.handle);
    if (buf.handle == 0) return buf;

    glBindBuffer(GL_ARRAY_BUFFER, buf.handle);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, GL_STATIC_DRAW);
    if (glGetError() != GL_NO_ERROR) {
        glDeleteBuffers(1, &buf.handle);
        buf.handle = 0;
        return buf;
    }

    buf.size = size;
    return buf;
}

RHIBuffer RHI::CreateIndexBuffer(const void* data, size_t size) {
    RHIBuffer buf;
    if (!initialized_ || data == nullptr || size == 0) return buf;

    glGenBuffers(1, &buf.handle);
    if (buf.handle == 0) return buf;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.handle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, GL_STATIC_DRAW);
    if (glGetError() != GL_NO_ERROR) {
        glDeleteBuffers(1, &buf.handle);
        buf.handle = 0;
        return buf;
    }

    buf.size = size;
    return buf;
}

void RHI::DestroyBuffer(RHIBuffer& buffer) {
    if (buffer.handle != 0) {
        glDeleteBuffers(1, &buffer.handle);
    }
    buffer.handle = 0;
    buffer.size = 0;
}

void RHI::BeginFrame() {
    if (!initialized_) return;
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RHI::DrawIndexed(const RHIMesh& mesh, uint32_t instanceCount) {
    if (!initialized_ ||
        mesh.vertexBuffer.handle == 0 ||
        mesh.indexBuffer.handle == 0 ||
        mesh.indexCount == 0 ||
        instanceCount == 0) {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBuffer.handle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBuffer.handle);
    glDrawElementsInstanced(
        GL_TRIANGLES,
        static_cast<GLsizei>(mesh.indexCount),
        GL_UNSIGNED_SHORT,
        nullptr,
        static_cast<GLsizei>(instanceCount));
}

void RHI::EndFrame() {
    if (!initialized_) return;
}

} // namespace NeoEngine
