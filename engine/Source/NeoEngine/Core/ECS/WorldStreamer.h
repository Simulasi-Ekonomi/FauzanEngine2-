#pragma once

#include <unordered_map>
#include <vector>
#include <cmath>
#include <functional>
#include "Entity.h"

namespace NeoEngine {

struct WorldChunk {
    int x, z;
    std::vector<Entity> entities;
    bool loaded = false;
};

class WorldStreamer {
public:
    using ChunkLoader = std::function<std::vector<Entity>(int, int)>;

    WorldStreamer(int chunkSize = 256, int loadDistance = 3)
        : m_ChunkSize(chunkSize), m_LoadDistance(loadDistance) {}

    void SetChunkLoader(ChunkLoader loader) { m_Loader = loader; }

    void Update(float cameraX, float cameraZ) {
        int camChunkX = static_cast<int>(std::floor(cameraX / m_ChunkSize));
        int camChunkZ = static_cast<int>(std::floor(cameraZ / m_ChunkSize));

        // Load chunks dalam radius
        for (int dx = -m_LoadDistance; dx <= m_LoadDistance; ++dx) {
            for (int dz = -m_LoadDistance; dz <= m_LoadDistance; ++dz) {
                int cx = camChunkX + dx;
                int cz = camChunkZ + dz;
                int64_t key = Hash(cx, cz);

                if (m_Chunks.find(key) == m_Chunks.end() && m_Loader) {
                    WorldChunk chunk{cx, cz};
                    chunk.entities = m_Loader(cx, cz);
                    chunk.loaded = true;
                    m_Chunks[key] = chunk;
                }
            }
        }

        // Unload chunks di luar radius
        auto it = m_Chunks.begin();
        while (it != m_Chunks.end()) {
            int dx = std::abs(it->second.x - camChunkX);
            int dz = std::abs(it->second.z - camChunkZ);
            if (dx > m_LoadDistance + 1 || dz > m_LoadDistance + 1) {
                it = m_Chunks.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::vector<Entity> GetVisibleEntities() const {
        std::vector<Entity> result;
        for (const auto& [key, chunk] : m_Chunks) {
            result.insert(result.end(), chunk.entities.begin(), chunk.entities.end());
        }
        return result;
    }

private:
    int64_t Hash(int x, int z) const {
        return (static_cast<int64_t>(x) << 32) | static_cast<uint32_t>(z);
    }

    int m_ChunkSize;
    int m_LoadDistance;
    std::unordered_map<int64_t, WorldChunk> m_Chunks;
    ChunkLoader m_Loader;
};

} // namespace NeoEngine
