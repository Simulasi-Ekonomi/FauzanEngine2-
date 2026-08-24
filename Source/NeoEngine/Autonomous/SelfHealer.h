#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <android/log.h>

namespace NeoEngine {

struct FaultReport;

class SelfHealer {
public:
    void Initialize() { m_Initialized = true; }

    bool Repair(const FaultReport& fault) {
        if (!m_Initialized) return false;
        
        auto it = m_HealStrategies.find(fault.component);
        if (it == m_HealStrategies.end()) {
            m_UnresolvedFaults++;
            return false;
        }

        auto& strategy = it->second;
        for (int attempt = 1; attempt <= strategy.maxRetries; attempt++) {
            if (strategy.delay.count() > 0) {
                std::this_thread::sleep_for(strategy.delay);
            }
            if (strategy.action()) {
                __android_log_print(ANDROID_LOG_INFO, "SelfHealer", 
                    "Component '%s' healed (attempt %d/%d)", 
                    fault.component.c_str(), attempt, strategy.maxRetries);
                return true;
            }
        }
        
        __android_log_print(ANDROID_LOG_ERROR, "SelfHealer", 
            "Failed to heal component '%s' after %d attempts",
            fault.component.c_str(), strategy.maxRetries);
        m_UnresolvedFaults++;
        return false;
    }

    void RegisterHealStrategy(const std::string& component, std::function<bool()> healAction,
                              int maxRetries = 3, std::chrono::milliseconds delay = std::chrono::milliseconds(0)) {
        m_HealStrategies[component] = {healAction, maxRetries, delay, 0};
    }

    bool IsHealthy() const { return m_UnresolvedFaults == 0; }
    int GetUnresolvedCount() const { return m_UnresolvedFaults; }

private:
    struct HealStrategy {
        std::function<bool()> action;
        int maxRetries;
        std::chrono::milliseconds delay;
        int retries = 0;
    };

    std::unordered_map<std::string, HealStrategy> m_HealStrategies;
    int m_UnresolvedFaults = 0;
    bool m_Initialized = false;
};

} // namespace NeoEngine
