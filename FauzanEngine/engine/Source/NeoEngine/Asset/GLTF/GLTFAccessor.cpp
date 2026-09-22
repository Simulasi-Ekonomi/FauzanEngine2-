#include "GLTFAccessor.h"
#include <cstring>
#include <limits>
namespace NeoEngine {
std::vector<float> GLTFAccessor::ReadFloatArray(const uint8_t* buffer,size_t offset,size_t count,size_t stride) {
    std::vector<float> result;
    if(!buffer || count==0 || stride<sizeof(float) || offset>std::numeric_limits<size_t>::max()-count*stride) return result;
    result.reserve(count);
    for(size_t i=0;i<count;++i){float value=0.0f; std::memcpy(&value,buffer+offset+i*stride,sizeof(float)); result.push_back(value);}
    return result;
}
}