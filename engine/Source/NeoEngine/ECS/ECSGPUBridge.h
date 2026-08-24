#pragma once
#include <cstddef>

namespace NeoEngine {

class ECSGPUBridge {
public:
    static void Initialize(const char* masterKey, const char* serverURL);
    static void SubmitToGPU(const void* data, size_t size);
};

} // namespace
