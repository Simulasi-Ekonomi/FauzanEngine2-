#pragma once

#include "MemoryTracker.h"

#define NE_ALLOC(size) NeoEngine::MemoryTracker::Allocate(size,__FILE__,__LINE__)
#define NE_FREE(ptr) NeoEngine::MemoryTracker::Deallocate(ptr)
