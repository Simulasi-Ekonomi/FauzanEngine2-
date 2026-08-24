#pragma once
#include <string>
#include <unordered_map>
#include <chrono>
#include <functional>

namespace NeoEngine {

class StatisticsTracker {
private:
    std::unordered_map<std::string, float> m_Counters;
    std::unordered_map<std::string, std::vector<float>> m_TimeSeries;
    std::chrono::system_clock::time_point m_StartTime;

public:
    StatisticsTracker() { m_StartTime = std::chrono::system_clock::now(); }

    void IncrementCounter(const std::string& name, float amount = 1) { m_Counters[name] += amount; }
    float GetCounter(const std::string& name) const { auto it = m_Counters.find(name); return it != m_Counters.end() ? it->second : 0; }
    void RecordValue(const std::string& name, float value) {
        m_TimeSeries[name].push_back(value);
        if (m_TimeSeries[name].size() > 1000) m_TimeSeries[name].erase(m_TimeSeries[name].begin());
    }
    float GetAverage(const std::string& name) const {
        auto it = m_TimeSeries.find(name);
        if (it == m_TimeSeries.end() || it->second.empty()) return 0;
        float sum = 0;
        for (auto v : it->second) sum += v;
        return sum / it->second.size();
    }
    int GetUptimeMinutes() const {
        return std::chrono::duration_cast<std::chrono::minutes>(
            std::chrono::system_clock::now() - m_StartTime).count();
    }
    void Reset() { m_Counters.clear(); m_TimeSeries.clear(); m_StartTime = std::chrono::system_clock::now(); }
};

} // namespace NeoEngine
