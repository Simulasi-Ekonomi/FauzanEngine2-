#pragma once
#include <cstdlib>
#include <cstdint>

namespace NeoEngine {

// INDUSTRY AAA MEMORY SAFE CHUNK
template<typename T, size_t CHUNK_SIZE = 1024>
class ArchetypeChunk {
private:
    uint8_t* data;

public:
    ArchetypeChunk() {
        data = reinterpret_cast<uint8_t*>(AlignedAlloc(64, sizeof(T) * CHUNK_SIZE));
    }

    ~ArchetypeChunk() {
        free(data);
    }

    static void* AlignedAlloc(size_t alignment, size_t size) {
        void* ptr = nullptr;
        if (posix_memalign(&ptr, alignment, size) != 0) return nullptr;
        return ptr;
    }

    T* GetData() {
        return reinterpret_cast<T*>(data);
    }
};

}
