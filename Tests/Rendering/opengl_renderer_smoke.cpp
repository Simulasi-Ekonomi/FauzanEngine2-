#include "Rendering/Renderer/Renderer.h"

#include <GLES3/gl3.h>
#include <array>
#include <cstdint>
#include <cstdio>

int main() {
    auto& renderer = NeoEngine::Renderer::Get();
    if (renderer.InitHeadless(0, 64) || renderer.IsInitialized()) return 1;
    if (!renderer.InitHeadless(64, 64) || !renderer.IsInitialized()) return 2;

    constexpr std::array<float, 9> vertices{{-1.0F, -1.0F, 0.0F, 1.0F, -1.0F, 0.0F, 0.0F, 1.0F, 0.0F}};
    constexpr std::array<unsigned int, 3> invalidIndices{{0, 1, 3}};
    constexpr std::array<unsigned int, 3> indices{{0, 1, 2}};
    if (renderer.CreateMesh(vertices.data(), 3, invalidIndices.data(), 3) != nullptr) return 3;
    auto* mesh = renderer.CreateMesh(vertices.data(), 3, indices.data(), 3);
    if (mesh == nullptr) return 4;

    constexpr std::array<unsigned char, 4> pixel{{255, 255, 255, 255}};
    if (renderer.CreateTexture(pixel.data(), 1, 1, 5) != nullptr) return 5;
    if (renderer.CreateTexture(pixel.data(), 1, 1, 4) == nullptr) return 6;

    constexpr char vertexShader[] = R"(#version 300 es
layout(location = 0) in vec3 position;
void main() { gl_Position = vec4(position, 1.0); }
)";
    constexpr char fragmentShader[] = R"(#version 300 es
precision mediump float;
out vec4 color;
void main() { color = vec4(1.0, 0.0, 0.0, 1.0); }
)";
    auto* shader = renderer.CreateShader(vertexShader, fragmentShader);
    if (shader == nullptr || renderer.CreateShader("invalid", fragmentShader) != nullptr) return 7;

    renderer.Clear(0.0F, 0.0F, 0.0F, 1.0F);
    renderer.DrawMesh(mesh, shader);
    glFinish();
    std::array<std::uint8_t, 4> output{};
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, output.data());
    if (glGetError() != GL_NO_ERROR || output[0] < 200 || output[1] > 40 || output[2] > 40) return 8;

    NeoEngine::Mesh foreignMesh{};
    NeoEngine::Shader foreignShader{};
    renderer.DrawMesh(&foreignMesh, &foreignShader);
    renderer.Shutdown();
    if (renderer.IsInitialized() || renderer.GetWidth() != 0 || renderer.GetHeight() != 0) return 9;
    if (!renderer.InitHeadless(32, 32)) return 10;
    renderer.Shutdown();

    std::puts("OPENGL_RENDERER_SMOKE_OK pbuffer=1 resource_lifecycle=1 shader=1 indexed_draw=1 pixel=1 reinitialize=1");
    return 0;
}
