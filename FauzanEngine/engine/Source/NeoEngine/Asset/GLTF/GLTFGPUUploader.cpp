#include "GLTFGPUUploader.h"
#include <cstring>
#include <limits>
namespace NeoEngine {
GPUBuffer GLTFGPUUploader::UploadMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    GPUBuffer buffer{};
    const size_t vertexBytes=vertices.size()*sizeof(Vertex), indexBytes=indices.size()*sizeof(uint32_t);
    if (vertexBytes > std::numeric_limits<size_t>::max()-indexBytes) return buffer;
    buffer.size=vertexBytes+indexBytes;
    if (buffer.size==0) return buffer;
    buffer.id=GPUBuffer::NextId();
    buffer.data.resize(buffer.size);
    if(vertexBytes) std::memcpy(buffer.data.data(),vertices.data(),vertexBytes);
    if(indexBytes) std::memcpy(buffer.data.data()+vertexBytes,indices.data(),indexBytes);
    return buffer;
}
}