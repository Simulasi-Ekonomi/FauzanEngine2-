#pragma once
#include <vector>
#include <cstdint>
#include <cassert>

namespace NeoEngine {

template<typename T>
class MemoryPool {
private:
    static constexpr uint32_t BLOCK_CAPACITY = 256;
    struct MemoryBlock {
        uint8_t* buffer;
        MemoryBlock(size_t elementSize) {
            buffer = static_cast<uint8_t*>(malloc(elementSize * BLOCK_CAPACITY));
            assert(buffer && "MemoryPool block allocation failed");
        }
        ~MemoryBlock() { free(buffer); }
        void* Get(size_t index) { return buffer + index * sizeof(T); }
    };
    std::vector<MemoryBlock*> blocks;
    std::vector<uint32_t> freeSlots;
    size_t elementSize = sizeof(T);

public:
    ~MemoryPool() {
        for (auto* block : blocks) delete block;
    }

    T* Allocate() {
        if (!freeSlots.empty()) {
            uint32_t index = freeSlots.back();
            freeSlots.pop_back();
            uint32_t blockIdx = index / BLOCK_CAPACITY;
            uint32_t slotIdx  = index % BLOCK_CAPACITY;
            return static_cast<T*>(blocks[blockIdx]->Get(slotIdx));
        }

        if (blocks.empty() || blocks.back()->Get(BLOCK_CAPACITY - 1) == nullptr) {
            blocks.push_back(new MemoryBlock(elementSize));
        }
        uint32_t blockIdx = blocks.size() - 1;
        uint32_t slotIdx  = (blocks.size() - 1) * BLOCK_CAPACITY + freeSlots.size();
        return static_cast<T*>(blocks[blockIdx]->Get(slotIdx));
    }

    void Deallocate(T* ptr) {
        // Sederhana: tambahkan ke freeSlots (produksi: perlu cari index)
        freeSlots.push_back(0); // Placeholder
    }
};

} // namespace NeoEngine
