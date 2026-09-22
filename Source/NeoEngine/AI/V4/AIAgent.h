#pragma once
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <cmath>
#include <limits>
#include <algorithm>

namespace NeoEngine {

struct MoEConfig {
    int numExperts = 4;
    int numSharedExperts = 2;
    int expertCapacity = 64;
    float routingTemp = 1.0f;
};

struct AIState {
    std::vector<float> hiddenState;
    int currentDepth = 0;
    bool finished = false;
    float confidence = 0.0f;
};

struct AIOutput {
    std::string action;
    std::vector<float> parameters;
    float confidence = 0.0f;
    bool success = false;
};

class AIExpert {
public:
    virtual ~AIExpert() = default;
    virtual void Train(const std::vector<float>& input, const std::vector<float>& target) {
        if (input.empty() || input.size() != target.size()) return;
        lastInput_ = input;
        lastTarget_ = target;
    }
    virtual std::vector<float> Predict(const std::vector<float>& input) {
        if (input.empty()) return {};
        std::vector<float> output(input.size());
        for (size_t i = 0; i < input.size(); ++i) output[i] = std::tanh(input[i]);
        return output;
    }
    std::string name;
    std::string specialization;
protected:
    std::vector<float> lastInput_;
    std::vector<float> lastTarget_;
};

class MoERouter {
public:
    int Route(const std::vector<float>& input, const std::vector<std::unique_ptr<AIExpert>>& experts) const {
        if (input.empty() || experts.empty()) return -1;
        int bestIdx = -1;
        float bestScore = -std::numeric_limits<float>::infinity();
        for (size_t i = 0; i < experts.size(); ++i) {
            if (!experts[i]) continue;
            float score = 0.0f;
            float norm = 0.0f;
            for (float v : input) {
                if (!std::isfinite(v)) return -1;
                score += v;
                norm += v * v;
            }
            score /= std::sqrt(std::max(norm, 1e-12f));
            score += static_cast<float>(i) * 1e-6f;
            if (score > bestScore) { bestScore = score; bestIdx = static_cast<int>(i); }
        }
        return bestIdx;
    }
};

class FauzanAIAgent {
public:
    FauzanAIAgent() { m_State.hiddenState.resize(64, 0.0f); }

    void ConfigureMoE(const MoEConfig& config) {
        m_Config = config;
        m_Experts.clear();
        m_SharedExperts.clear();
        const int experts = std::clamp(config.numExperts, 1, 256);
        const int shared = std::clamp(config.numSharedExperts, 0, 64);
        m_Experts.reserve(experts);
        m_SharedExperts.reserve(shared);
        for (int i = 0; i < experts; ++i) m_Experts.push_back(std::make_unique<AIExpert>());
        for (int i = 0; i < shared; ++i) m_SharedExperts.push_back(std::make_unique<AIExpert>());
    }

    AIState PreludePhase(const std::vector<float>& observation) {
        m_State.currentDepth = 0;
        m_State.finished = false;
        m_State.confidence = 0.0f;
        std::fill(m_State.hiddenState.begin(), m_State.hiddenState.end(), 0.0f);
        for (size_t i = 0; i < m_State.hiddenState.size() && i < observation.size(); ++i)
            if (std::isfinite(observation[i])) m_State.hiddenState[i] = observation[i];
        return m_State;
    }

    AIState RecurrentPhase(int maxDepth = 5) {
        maxDepth = std::clamp(maxDepth, 1, 1024);
        if (m_Experts.empty()) ConfigureMoE(m_Config);
        for (int step = 0; step < maxDepth && !m_State.finished; ++step) {
            const int selected = m_Router.Route(m_State.hiddenState, m_Experts);
            if (selected < 0) break;
            const std::vector<float> predicted = m_Experts[selected]->Predict(m_State.hiddenState);
            float delta = 0.0f;
            for (size_t i = 0; i < m_State.hiddenState.size() && i < predicted.size(); ++i) {
                const float next = 0.9f * m_State.hiddenState[i] + 0.1f * predicted[i];
                delta = std::max(delta, std::fabs(next - m_State.hiddenState[i]));
                m_State.hiddenState[i] = next;
            }
            m_State.currentDepth = step + 1;
            m_State.confidence = std::clamp(1.0f - delta, 0.0f, 1.0f);
            m_State.finished = delta < 1e-4f || step + 1 == maxDepth;
        }
        return m_State;
    }

    AIOutput CodaPhase() {
        AIOutput output;
        output.parameters = m_State.hiddenState;
        if (!m_State.finished || output.parameters.empty()) return output;
        const float mean = std::accumulate(output.parameters.begin(), output.parameters.end(), 0.0f) /
                           static_cast<float>(output.parameters.size());
        output.action = mean > 0.25f ? "advance" : (mean < -0.25f ? "retreat" : "idle");
        output.confidence = m_State.confidence;
        output.success = true;
        return output;
    }

    AIOutput Think(const std::vector<float>& observation, int maxDepth = 5) {
        if (observation.empty()) return {};
        PreludePhase(observation);
        RecurrentPhase(maxDepth);
        return CodaPhase();
    }

private:
    MoEConfig m_Config;
    AIState m_State;
    MoERouter m_Router;
    std::vector<std::unique_ptr<AIExpert>> m_Experts;
    std::vector<std::unique_ptr<AIExpert>> m_SharedExperts;
};

} // namespace NeoEngine
