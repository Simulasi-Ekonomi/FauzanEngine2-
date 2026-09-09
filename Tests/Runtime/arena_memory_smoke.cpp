#include "Core/Memory/Arena.h"
#include <cassert>
#include <cstdint>
#include <limits>
#include <utility>

int main() {
    Neo::Arena empty(0);
    assert(empty.Alloc(1) == nullptr);
    assert(empty.GetUsedMemory() == 0);

    Neo::Arena arena(128);
    assert(arena.GetCapacity() == 128);
    assert(arena.Alloc(1, 0) == nullptr);
    assert(arena.Alloc(1, 3) == nullptr);

    void* aligned = arena.Alloc(7, 16);
    assert(aligned != nullptr);
    assert(reinterpret_cast<std::uintptr_t>(aligned) % 16 == 0);
    assert(arena.GetUsedMemory() >= 7);

    const std::size_t used = arena.GetUsedMemory();
    assert(arena.Alloc(std::numeric_limits<std::size_t>::max(), 16) == nullptr);
    assert(arena.GetUsedMemory() == used);

    arena.Reset();
    assert(arena.GetUsedMemory() == 0);
    assert(arena.Alloc(128, 1) != nullptr);
    assert(arena.Alloc(1, 1) == nullptr);

    Neo::Arena moved(std::move(arena));
    assert(arena.Alloc(1) == nullptr);
    assert(moved.GetUsedMemory() == 128);

    Neo::Arena assigned(8);
    assigned = std::move(moved);
    assert(moved.Alloc(1) == nullptr);
    assert(assigned.GetUsedMemory() == 128);

    return 0;
}
