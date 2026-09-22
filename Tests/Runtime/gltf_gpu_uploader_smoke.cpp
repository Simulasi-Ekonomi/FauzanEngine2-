#include "Asset/GLTF/GLTFGPUUploader.h"
#include <cstdio>

int main() {
    NeoEngine::GLTFGPUUploader uploader;
    if (uploader.IsValid()) return 1;
    if (uploader.UploadMesh({}, {})) return 2;
    std::puts("GLTF_GPU_UPLOADER_SMOKE_OK validation=1");
    return 0;
}
