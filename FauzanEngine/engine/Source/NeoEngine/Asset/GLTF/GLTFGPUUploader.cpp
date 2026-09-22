#include "GLTFGPUUploader.h"
#include <cstring>
#include <limits>
namespace NeoEngine { GPUBuffer GLTFGPUUploader::UploadMesh(const std::vector<Vertex>&v,const std::vector<uint32_t>&i){GPUBuffer b;size_t vb=v.size()*sizeof(Vertex),ib=i.size()*sizeof(uint32_t);if(vb>std::numeric_limits<size_t>::max()-ib)return b;b.size=vb+ib;if(!b.size)return b;b.id=GPUBuffer::NextId();b.data.resize(b.size);if(vb)std::memcpy(b.data.data(),v.data(),vb);if(ib)std::memcpy(b.data.data()+vb,i.data(),ib);return b;} }