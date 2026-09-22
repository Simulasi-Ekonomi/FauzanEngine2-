#include "GLTFAccessor.h"
#include <cstring>
#include <limits>
namespace NeoEngine {std::vector<float>GLTFAccessor::ReadFloatArray(const uint8_t*b,size_t o,size_t c,size_t s){std::vector<float>r;if(!b||!c||s<4||o>std::numeric_limits<size_t>::max()-c*s)return r;r.reserve(c);for(size_t i=0;i<c;++i){float v=0;std::memcpy(&v,b+o+i*s,4);r.push_back(v);}return r;}}