#include <cassert>
#include "MemoryManager.h"
#include <cstdlib>

void MemoryManager::Init() {
    // Placeholder untuk inisialisasi tracking memori global
}

void MemoryManager::Shutdown() {
    // Placeholder untuk report memory leak di akhir lifecycle
}

void* MemoryManager::Allocate(size_t size) {
    // Wrapper awal atas OS Allocator
    return std::malloc(size);
}

void MemoryManager::Free(void* ptr) {
    if (ptr) std::free(ptr);
}
