#pragma once

#include <unordered_map>
#include <vector>
#include <cstdint>

#include "Entity.h"

class VirtualEntityPool
{

private:

    static const uint32_t PAGE_SIZE = 65536;

    struct Page
    {
        [[maybe_unused]] std::vector<Entity> entities;
    };

    std::unordered_map<uint32_t, Page> pages;

    uint64_t totalEntities = 0;

public:

    Entity Create()
    {

        uint32_t pageIndex = totalEntities / PAGE_SIZE;
        uint32_t slotIndex = totalEntities % PAGE_SIZE;

        if (pages.find(pageIndex) == pages.end())
        {
            pages[pageIndex].entities.resize(PAGE_SIZE);
        }

        Entity e;
        e.index = totalEntities;
        e.version = 0;

        pages[pageIndex].entities[slotIndex] = e;

        totalEntities++;

        return e;

    }

    Entity* Get(uint64_t id)
    {

        uint32_t pageIndex = id / PAGE_SIZE;
        uint32_t slotIndex = id % PAGE_SIZE;

        if (pages.find(pageIndex) == pages.end())
            return nullptr;

        return &pages[pageIndex].entities[slotIndex];

    }

    uint64_t Size() const
    {
        return totalEntities;
    }

};
