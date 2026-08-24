#include <cstdlib>
#include <cstddef>
#include <new>

#ifdef __ANDROID__
#include <android/log.h>
#endif

// Checked allocator wrapper - tracks allocations and validates frees
static size_t GAllocCount = 0;
static size_t GTotalAllocated = 0;

void* NeoMalloc(size_t Size) {
    if (Size == 0) return nullptr;
    void* ptr = std::malloc(Size);
    if (!ptr) {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_ERROR, "NeoMemory",
                            "Allocation failed for %zu bytes", Size);
#endif
        throw std::bad_alloc();
    }
    GAllocCount++;
    GTotalAllocated += Size;
    return ptr;
}

void NeoFree(void* Ptr) {
    if (!Ptr) return;
    GAllocCount--;
    std::free(Ptr);
}

size_t NeoGetAllocCount() { return GAllocCount; }
size_t NeoGetTotalAllocated() { return GTotalAllocated; }
