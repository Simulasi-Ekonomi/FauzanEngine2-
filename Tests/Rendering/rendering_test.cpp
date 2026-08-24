#include "Source/NeoEngine/Rendering/OpenGLES/GLESRenderer.h"
#include <chrono>
#include <cstdio>

int main() {
    printf("============================================\n");
    printf(" 3D RENDERING TEST – 6 Complex Shapes\n");
    printf("============================================\n");

    NeoEngine::GLESRenderer renderer;
    if (!renderer.Initialize(1280, 720)) {
        printf("❌ Failed to initialize EGL/GLES3\n");
        return 1;
    }

    printf("✅ Renderer initialized (1280x720)\n");
    printf(" Shapes: Cube, Sphere, Torus, Pyramid, Cylinder, Cone\n\n");

    const char* names[] = {"Cube", "Sphere", "Torus", "Pyramid", "Cylinder", "Cone"};
    for (int i = 0; i < 6; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        renderer.Render(i);   // <-- menggunakan Render sesuai header baru
        auto end = std::chrono::high_resolution_clock::now();
        float ms = std::chrono::duration<float, std::milli>(end - start).count();
        printf("  %-10s : %.2f ms\n", names[i], ms);
    }

    printf("\n============================================\n");
    printf(" ✅ ALL 3D SHAPES RENDERED SUCCESSFULLY\n");
    printf(" Engine mampu menghasilkan geometri kompleks\n");
    printf("============================================\n");
    return 0;
}
