#pragma once
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <unordered_map>

namespace NeoEngine {

struct RLConfig {
    float gamma = 0.99f;          // discount factor
    float epsilon = 0.1f;         // exploration rate
    float epsilonDecay = 0.995f;  // decay per episode
    float learningRate = 0.001f;
    int stateSize = 10;
    int actionSize = 4;
};

class RLAgent {
public:
    RLAgent(const RLConfig& config) : m_Config(config) {
        // Inisialisasi Q-table (state sederhana: indeks diskrit)
        m_QTable.resize(1000, std::vector<float>(config.actionSize, 0.0f));
        m_RNG.seed(42);
    }

    // Pilih aksi (epsilon-greedy)
    int SelectAction(const std::vector<float>& state) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        if (dist(m_RNG) < m_Config.epsilon) {
            return std::uniform_int_distribution<int>(0, m_Config.actionSize - 1)(m_RNG);
        }
        int idx = DiscretizeState(state);
        auto& values = m_QTable[idx];
        return static_cast<int>(std::distance(values.begin(),
            std::max_element(values.begin(), values.end())));
    }

    // Update Q-table (Q-learning)
    void Update(const std::vector<float>& state, int action, float reward,
                const std::vector<float>& nextState) {
        int s = DiscretizeState(state);
        int ns = DiscretizeState(nextState);
        float maxNext = *std::max_element(m_QTable[ns].begin(), m_QTable[ns].end());
        float& current = m_QTable[s][action];
        current += m_Config.learningRate * (reward + m_Config.gamma * maxNext - current);
    }

    // Decay epsilon setelah episode
    void DecayEpsilon() { m_Config.epsilon *= m_Config.epsilonDecay; }

    float GetEpsilon() const { return m_Config.epsilon; }

private:
    RLConfig m_Config;
    std::vector<std::vector<float>> m_QTable;
    std::mt19937 m_RNG;

    int DiscretizeState(const std::vector<float>& state) {
        // Konversi state kontinu ke indeks diskrit (placeholder)
        int idx = 0;
        for (size_t i = 0; i < state.size() && i < 10; ++i) {
            int bucket = static_cast<int>(state[i] * 10.0f) % 100;
            idx = idx * 100 + bucket;
        }
        return std::abs(idx) % static_cast<int>(m_QTable.size());
    }
};

} // namespace NeoEngine
