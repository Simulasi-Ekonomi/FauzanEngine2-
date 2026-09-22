#include "GLTFGPUUploader.h"
#include "../Core/GPU/GPUBufferPool.h"
#include <limits>
namespace NeoEngine {
GPUBuffer GLTFGPUUploader::UploadMesh(const std::vector<Vertex>& vertices,const std::vector<uint32_t>& indices){
    const size_t bytes=vertices.size()*sizeof(Vertex)+indices.size()*sizeof(uint32_t);
    if(bytes==0 || bytes>std::numeric_limits<size_t>::max()) return GPUBuffer{};
    GPUBufferPool pool; return pool.Allocate(bytes);
}
}
