#pragma once
#include "AIAgent.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace NeoEngine {

// Pelatihan AI online (inspirasi GLM-5 agentic training)
class AITrainer {
public:
    void CollectExperience(const std::vector<float>& observation,
                           const std::string& action,
                           float reward) {
        m_Experience.push_back({observation, action, reward});
    }

    // Update agent berdasarkan pengalaman (Q-learning sederhana)
    void TrainAgent(FauzanAIAgent& agent) {
        for (auto& exp : m_Experience) {
            // Placeholder: update bobot agent berdasarkan reward
            // Implementasi sesungguhnya akan menggunakan backpropagation atau
            // reinforcement learning sesuai kebutuhan.
        }
        m_Experience.clear();
    }

    size_t GetExperienceCount() const { return m_Experience.size(); }

private:
    struct Experience {
        std::vector<float> observation;
        std::string action;
        float reward;
    };
    std::vector<Experience> m_Experience;
};

} // namespace NeoEngine
