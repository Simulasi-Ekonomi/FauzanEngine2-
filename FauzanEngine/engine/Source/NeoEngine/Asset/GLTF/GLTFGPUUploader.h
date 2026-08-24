#include <cassert>
#pragma once
#include <vector>
#include "GPUBuffer.h"
#include "Vertex.h"

namespace NeoEngine {

class GLTFGPUUploader {
public:
    GPUBuffer UploadMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
};

} // namespace NeoEngine
