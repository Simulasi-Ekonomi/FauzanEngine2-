#include "EngineAllocator.h"

void* operator new(size_t size) {
    return Neo::EngineAllocator::Allocate(size);
}

void operator delete(void* p) noexcept {
    Neo::EngineAllocator::Deallocate(p);
}
