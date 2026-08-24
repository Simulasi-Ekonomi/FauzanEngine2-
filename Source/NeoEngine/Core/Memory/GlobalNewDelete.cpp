#include "EngineAllocator.h"

void* operator new(std::size_t size) {
    return NeoEngine::EngineAllocator::Allocate(size);
}

void* operator new[](std::size_t size) {
    return NeoEngine::EngineAllocator::Allocate(size);
}

void operator delete(void* p) noexcept {
    NeoEngine::EngineAllocator::Deallocate(p);
}

void operator delete[](void* p) noexcept {
    NeoEngine::EngineAllocator::Deallocate(p);
}

void operator delete(void* p, std::size_t) noexcept {
    NeoEngine::EngineAllocator::Deallocate(p);
}

void operator delete[](void* p, std::size_t) noexcept {
    NeoEngine::EngineAllocator::Deallocate(p);
}
