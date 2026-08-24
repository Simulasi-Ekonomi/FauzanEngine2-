#include "RHI.h"
#include <GLES3/gl31.h>
#include <android/log.h>

#define LOG_TAG "NeoRHI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

RHI& RHI::Get() {
    static RHI instance;
    return instance;
}

bool RHI::Initialize() {
    if (initialized_) return true;
    LOGI("RHI Initialized - OpenGL ES 3.1");
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    initialized_ = true;
    return true;
}

void RHI::Shutdown() {
    initialized_ = false;
}

RHIBuffer RHI::CreateVertexBuffer(const void* data, size_t size) {
    RHIBuffer buf;
    glGenBuffers(1, &buf.handle);
    glBindBuffer(GL_ARRAY_BUFFER, buf.handle);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    buf.size = size;
    return buf;
}

RHIBuffer RHI::CreateIndexBuffer(const void* data, size_t size) {
    RHIBuffer buf;
    glGenBuffers(1, &buf.handle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.handle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    buf.size = size;
    return buf;
}

void RHI::DestroyBuffer(RHIBuffer& buffer) {
    if (buffer.handle) glDeleteBuffers(1, &buffer.handle);
    buffer.handle = 0;
    buffer.size = 0;
}

void RHI::BeginFrame() {
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RHI::DrawIndexed(const RHIMesh& mesh, uint32_t instanceCount) {
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBuffer.handle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBuffer.handle);
    // Attribute pointers akan di-set oleh shader system nantinya
    glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_SHORT, 0, instanceCount);
}

void RHI::EndFrame() {
    // EGL swap buffers handled by Android
}

} // namespace
