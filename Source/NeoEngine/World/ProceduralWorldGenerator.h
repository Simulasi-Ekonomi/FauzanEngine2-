#pragma once
#include "FastNoiseLite.h"
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <algorithm>
#include <random>
#include "Core/Math/Vector3.h"

namespace NeoEngine {

enum class BuildingType { CITY_HUMAN, CITY_ELF, VILLAGE, RUINS, FARM };

struct CityCenter {
    float x, y, z;
    BuildingType type;
    float radius;
};

struct ResourceNode {
    float x, y, z;
    std::string type;
};

struct MonsterSpawn {
    float x, y, z;
    std::string type;
    int level;
};

struct WorldConfig {
    float worldSizeKm = 10.0f;
    float treeDensity = 50.0f;
    float rockDensity = 30.0f;
    float buildingDensity = 2.0f;
    int cityCount = 3;
    unsigned int seed = 12345;
};

class ProceduralWorldGenerator {
public:
    ProceduralWorldGenerator();
    void Generate(const WorldConfig& config);
    const std::vector<Vector3>& GetTrees() const { return m_Trees; }
    const std::vector<Vector3>& GetRocks() const { return m_Rocks; }
    const std::vector<ResourceNode>& GetResources() const { return m_Resources; }
    const std::vector<MonsterSpawn>& GetMonsters() const { return m_Monsters; }
    const std::vector<CityCenter>& GetCityCenters() const { return m_CityCenters; }

private:
    void GenerateCityCenters();
    bool IsNearCivilization(float wx, float wz) const;

    FastNoiseLite m_Noise;
    WorldConfig m_Config;
    std::vector<Vector3> m_Trees;
    std::vector<Vector3> m_Rocks;
    std::vector<ResourceNode> m_Resources;
    std::vector<MonsterSpawn> m_Monsters;
    std::vector<CityCenter> m_CityCenters;
    std::mt19937 m_RNG;
};

} // namespace NeoEngine
