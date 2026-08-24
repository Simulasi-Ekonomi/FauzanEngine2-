#pragma once
#include "MemoryTracker.h"

// Makro pelacakan alokasi memori
#ifdef NEO_MEMORY_TRACKING
    #define NE_ALLOC(size) NeoEngine::MemoryTracker::Allocate(size, __FILE__, __LINE__)
    #define NE_FREE(ptr)   NeoEngine::MemoryTracker::Deallocate(ptr)
    #define NE_REALLOC(ptr, size) NeoEngine::MemoryTracker::Reallocate(ptr, size, __FILE__, __LINE__)
#else
    #define NE_ALLOC(size) malloc(size)
    #define NE_FREE(ptr)   free(ptr)
    #define NE_REALLOC(ptr, size) realloc(ptr, size)
#endif
