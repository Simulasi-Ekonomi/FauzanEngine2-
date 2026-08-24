#include "ProceduralWorldGenerator.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace NeoEngine {

ProceduralWorldGenerator::ProceduralWorldGenerator()
{
    m_Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
}

void ProceduralWorldGenerator::Generate(const WorldConfig& config) {
    m_Config = config;
    m_RNG.seed(config.seed);
    m_Noise.SetSeed(config.seed);

    float totalAreaKm2 = config.worldSizeKm * config.worldSizeKm;
    size_t totalTrees = static_cast<size_t>(config.treeDensity * totalAreaKm2);
    size_t totalRocks = static_cast<size_t>(config.rockDensity * totalAreaKm2);

    m_Trees.reserve(totalTrees);
    m_Rocks.reserve(totalRocks);

    GenerateCityCenters();

    float half = config.worldSizeKm * 500.0f;  // 1 km = 1000 unit, setengah dunia dalam unit
    std::uniform_real_distribution<float> posDist(-half, half);

    for (size_t i = 0; i < totalTrees; ++i) {
        float wx = posDist(m_RNG), wz = posDist(m_RNG);
        if (IsNearCivilization(wx, wz)) continue;
        float h = m_Noise.GetNoise(wx * 0.01f, wz * 0.01f) * 50.0f;
        m_Trees.push_back({wx, h, wz});
    }

    for (size_t i = 0; i < totalRocks; ++i) {
        float wx = posDist(m_RNG), wz = posDist(m_RNG);
        if (IsNearCivilization(wx, wz)) continue;
        float h = m_Noise.GetNoise(wx * 0.05f, wz * 0.05f) * 20.0f;
        m_Rocks.push_back({wx, h, wz});
    }
}

void ProceduralWorldGenerator::GenerateCityCenters() {
    float half = m_Config.worldSizeKm * 400.0f;
    std::uniform_real_distribution<float> posDist(-half, half);
    std::uniform_int_distribution<int> typeDist(0, 4);

    for (int i = 0; i < m_Config.cityCount; ++i) {
        CityCenter c;
        c.x = posDist(m_RNG);
        c.z = posDist(m_RNG);
        c.y = 0.0f;
        c.type = static_cast<BuildingType>(typeDist(m_RNG));
        c.radius = 200.0f + (m_RNG() % 300);
        m_CityCenters.push_back(c);
    }
}

bool ProceduralWorldGenerator::IsNearCivilization(float wx, float wz) const {
    for (const auto& c : m_CityCenters) {
        float dx = wx - c.x, dz = wz - c.z;
        if (NeoEngine::Math::Sqrt(dx*dx + dz*dz) < c.radius) return true;
    }
    return false;
}

} // namespace NeoEngine
