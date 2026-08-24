#pragma once
#include "MemoryBlock.h"
#include <vector>
#include <cstdint>

class MemoryPool {
private:
    static constexpr size_t BLOCK_CAPACITY = 1024;

    size_t elementSize;
    [[maybe_unused]] std::vector<MemoryBlock*> blocks;
    [[maybe_unused]] std::vector<uint32_t> freeSlots;

public:
    explicit MemoryPool(size_t elementSize)
        : elementSize(elementSize)
    {}

    ~MemoryPool() {
        for (auto* block : blocks)
            delete block;
    }

    void* Allocate() {
        if (!freeSlots.empty()) {
            uint32_t index = freeSlots.back();
            freeSlots.pop_back();

            uint32_t blockIndex = index / BLOCK_CAPACITY;
            uint32_t slotIndex  = index % BLOCK_CAPACITY;

            return blocks[blockIndex]->Get(slotIndex);
        }

        uint32_t totalSlots = blocks.size() * BLOCK_CAPACITY;
        if (freeSlots.empty() && totalSlots == blocks.size() * BLOCK_CAPACITY) {
            blocks.push_back(new MemoryBlock(elementSize, BLOCK_CAPACITY));
        }

        uint32_t blockIndex = blocks.size() - 1;
        uint32_t slotIndex  = totalSlots % BLOCK_CAPACITY;

        return blocks[blockIndex]->Get(slotIndex);
    }

    void Deallocate(void* ptr) {
        // Simplified for now
    }

    void* GetPointer(uint32_t blockIndex, uint32_t slotIndex) {
        return blocks[blockIndex]->Get(slotIndex);
    }

    void ResolveLocation(void* ptr, uint32_t& blockIndex, uint32_t& slotIndex) {
        for (uint32_t i = 0; i < blocks.size(); ++i) {
            uint8_t* start = static_cast<uint8_t*>(blocks[i]->Get(0));
            uint8_t* end   = start + BLOCK_CAPACITY * elementSize;

            if (ptr >= start && ptr < end) {
                blockIndex = i;
                slotIndex  = (static_cast<uint8_t*>(ptr) - start) / elementSize;
                return;
            }
        }
    }

    void DeallocateByLocation(uint32_t blockIndex, uint32_t slotIndex) {
        freeSlots.push_back(blockIndex * BLOCK_CAPACITY + slotIndex);
    }
};
