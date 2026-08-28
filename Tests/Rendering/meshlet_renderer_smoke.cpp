#include "Renderer/MeshletRenderer.h"
#include <cstdio>

int main() {
    MeshletRenderer renderer;
    if (!renderer.TryAddMeshlet({0, 8, 0, 12}) || !renderer.TryAddMeshlet({8, 4, 12, 6})) return 1;
    if (renderer.MeshletCount() != 2 || renderer.TriangleCount() != 18 || !renderer.Validate()) return 2;
    if (renderer.TryAddMeshlet({0, 0, 0, 1}) || renderer.TryAddMeshlet({0, 2, 0, 5})) return 3;
    renderer.Render();
    renderer.Clear();
    if (renderer.MeshletCount() != 0 || renderer.TriangleCount() != 0 || !renderer.Validate()) return 4;
    std::puts("MESHLET_RENDERER_SMOKE_OK validation=1 accounting=1 invalid_reject=1 clear=1");
    return 0;
}
