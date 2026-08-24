#include "MemoryManager.h"
#include "../Debug/MemoryTracker.h"
#include "../Debug/Logger.h"
#include <cstdlib>

void MemoryManager::Init() {
    NeoEngine::Logger::Init();
}

void MemoryManager::Shutdown() {
    NeoEngine::MemoryTracker::Report();
    NeoEngine::Logger::Shutdown();
}

void* MemoryManager::Allocate(size_t size) {
    return NeoEngine::MemoryTracker::Allocate(size, "MemoryManager", 0);
}

void MemoryManager::Free(void* ptr) {
    NeoEngine::MemoryTracker::Deallocate(ptr);
}
