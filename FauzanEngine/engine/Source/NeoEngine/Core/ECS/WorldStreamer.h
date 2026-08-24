#pragma once

#include <unordered_map>
#include <vector>
#include <cmath>

#include "Entity.h"

struct WorldChunk
{
    [[maybe_unused]] int x;
    [[maybe_unused]] int y;

    [[maybe_unused]] std::vector<Entity> entities;

};

class WorldStreamer
{

private:

    std::unordered_map<int64_t, WorldChunk> chunks;

    int chunkSize = 512;

    int64_t Hash(int x, int y)
    {
        return ((int64_t)x << 32) | (uint32_t)y;
    }

public:

    WorldChunk& GetChunk(int x, int y)
    {

        int64_t key = Hash(x,y);

        return chunks[key];

    }

    void AddEntity(Entity e)
    {

        int cx = std::floor(px / chunkSize);
        int cy = std::floor(py / chunkSize);

        auto& chunk = GetChunk(cx,cy);

        chunk.entities.push_back(e);

    }

    std::vector<Entity>* GetVisibleChunk(float px, float py)
    {

        int cx = std::floor(px / chunkSize);
        int cy = std::floor(py / chunkSize);

        int64_t key = Hash(cx,cy);

        if (chunks.find(key) == chunks.end())
            return nullptr;

        return &chunks[key].entities;

    }

};
