#pragma once
#include <string>
#include <unordered_map>
#include <chrono>

namespace NeoEngine {

class StatisticsTracker {
private:
    std::unordered_map<std::string, float> m_Counters;
    std::chrono::system_clock::time_point m_StartTime;

public:
    StatisticsTracker() { m_StartTime = std::chrono::system_clock::now(); }
    void IncrementCounter(const std::string& name, float amount = 1) { m_Counters[name] += amount; }
    float GetCounter(const std::string& name) const { 
        auto it = m_Counters.find(name); return it != m_Counters.end() ? it->second : 0; 
    }
};

} // namespace NeoEngine
