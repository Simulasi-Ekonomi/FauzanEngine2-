#include "RLAgent.h"
#pragma once
#include <vector>
#include <string>
#include <functional>
#include <chrono>

namespace NeoEngine {

struct RLSimConfig {
    int maxStepsPerEpisode = 1000;
    int totalEpisodes = 100;
    int saveInterval = 10;
    bool verbose = true;
};

class RLSimulator {
public:
    using RewardFunc = std::function<float(const std::vector<float>&, int)>;
    using StepFunc = std::function<std::vector<float>(const std::vector<float>&, int)>;
    using IsDoneFunc = std::function<bool(const std::vector<float>&)>;
    using ResetFunc = std::function<std::vector<float>()>;

    void SetRewardFunction(RewardFunc f) { m_RewardFunc = f; }
    void SetStepFunction(StepFunc f) { m_StepFunc = f; }
    void SetIsDoneFunction(IsDoneFunc f) { m_IsDoneFunc = f; }
    void SetResetFunction(ResetFunc f) { m_ResetFunc = f; }

    void RunEpisode(RLAgent& agent, int maxSteps) {
        if (!m_ResetFunc || !m_StepFunc || !m_IsDoneFunc || !m_RewardFunc) return;

        std::vector<float> state = m_ResetFunc();
        float totalReward = 0.0f;

        for (int step = 0; step < maxSteps; ++step) {
            int action = agent.SelectAction(state);
            std::vector<float> nextState = m_StepFunc(state, action);
            float reward = m_RewardFunc(nextState, action);
            bool done = m_IsDoneFunc(nextState);

            agent.Update(state, action, reward, nextState);
            state = nextState;
            totalReward += reward;

            if (done) break;
        }

        if (m_Config.verbose) {
            // Placeholder: log episode reward
        }
    }

    void Train(RLAgent& agent, const RLSimConfig& config) {
        for (int episode = 0; episode < config.totalEpisodes; ++episode) {
            RunEpisode(agent, config.maxStepsPerEpisode);
            agent.DecayEpsilon();
        }
    }

private:
    RLSimConfig m_Config;
    RewardFunc m_RewardFunc;
    StepFunc m_StepFunc;
    IsDoneFunc m_IsDoneFunc;
    ResetFunc m_ResetFunc;
};

} // namespace NeoEngine
