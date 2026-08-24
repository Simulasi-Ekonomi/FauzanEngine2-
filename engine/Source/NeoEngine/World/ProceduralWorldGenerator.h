#pragma once
#include <FastNoiseLite.h>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <cmath>
#include <algorithm>
#include <random>
#include "Core/Math/Vector3.h"

namespace NeoEngine {

struct WorldConfig {
    float worldSizeKm = 100.0f;
    float chunkSizeM = 256.0f;
    int seed = 12345;
    float treeDensity = 0.03f;
    float rockDensity = 0.01f;
    float buildingDensity = 0.002f;
    float heightScale = 200.0f;
};

struct Biome {
    std::string name;
    float minHeight, maxHeight;
    float minMoisture, maxMoisture;
    float treeDensityMod = 1.0f;
    std::vector<std::string> treeTypes;
    std::vector<std::string> groundObjects;
};

struct PlacedObject {
    std::string type;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale{1,1,1};
    uint32_t instanceID;
};

struct WorldChunk {
    int x, z;
    std::vector<PlacedObject> objects;
    float terrainHeight[64][64];  // heightmap per chunk
    Biome biome;
    bool generated = false;
};

class ProceduralWorldGenerator {
public:
    ProceduralWorldGenerator(const WorldConfig& cfg);
    ~ProceduralWorldGenerator();

    // Generate chunk on demand
    WorldChunk GenerateChunk(int chunkX, int chunkZ);

    // Precompute world metadata (biome map, rivers, roads)
    void PrecomputeWorld();

    // Get biome at world position
    Biome GetBiomeAt(float worldX, float worldZ);

    // Save/Load world state
    void SaveToFile(const std::string& path);
    void LoadFromFile(const std::string& path);

    const WorldConfig& GetConfig() const { return m_Config; }

private:
    WorldConfig m_Config;
    FastNoiseLite m_HeightNoise;
    FastNoiseLite m_MoistureNoise;
    FastNoiseLite m_TemperatureNoise;
    FastNoiseLite m_RiverNoise;
    std::vector<Biome> m_Biomes;
    std::mt19937 m_RNG;
    
    // Precomputed biome grid (low-res)
    std::unordered_map<uint64_t, Biome> m_BiomeMap;
    std::unordered_map<uint64_t, float> m_RiverMap;
    static constexpr int BIOME_GRID_SIZE = 64; // meters per cell

    void InitNoise();
    void DefineBiomes();
    float GetHeight(float x, float z);
    float GetMoisture(float x, float z);
    float GetTemperature(float x, float z);
    bool IsRiver(float x, float z);
    std::string SelectTreeType(const Biome& biome, float rand);
    uint64_t Hash(int x, int z) const;
};

} // namespace NeoEngine
