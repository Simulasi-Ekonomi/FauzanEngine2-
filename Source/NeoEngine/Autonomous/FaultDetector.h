#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <algorithm>

namespace NeoEngine {

struct FaultReport {
    std::string component;
    std::string description;
    int severity = 1; // 1-5
    std::chrono::system_clock::time_point timestamp;
    bool resolved = false;
    std::chrono::system_clock::time_point resolvedAt;
    int occurrences = 1;

    bool operator==(const FaultReport& other) const {
        return component == other.component && description == other.description;
    }
};

class FaultDetector {
public:
    void Initialize() { m_Initialized = true; }

    // Menganalisis untuk mencari kesalahan
    void Analyze() {
        if (!m_Initialized) return;
        for (auto& monitor : m_Monitors) {
            if (monitor.check && monitor.check()) {
                FaultReport fault;
                fault.component = monitor.component;
                fault.description = "Fault detected in " + monitor.component;
                fault.severity = monitor.severity;
                fault.timestamp = std::chrono::system_clock::now();
                
                // Cek apakah fault sudah ada
                auto it = std::find_if(m_ActiveFaults.begin(), m_ActiveFaults.end(),
                    [&](const FaultReport& f) { return f == fault; });
                if (it != m_ActiveFaults.end()) {
                    it->occurrences++;
                    it->timestamp = fault.timestamp;
                } else {
                    m_ActiveFaults.push_back(fault);
                }
            }
        }
    }

    bool HasFault() const { return !m_ActiveFaults.empty(); }
    std::vector<FaultReport> GetActiveFaults() const { return m_ActiveFaults; }
    FaultReport GetFaultReport() const { 
        return m_ActiveFaults.empty() ? FaultReport{} : m_ActiveFaults[0]; 
    }

    void ResolveFault(const FaultReport& fault) {
        auto it = std::find(m_ActiveFaults.begin(), m_ActiveFaults.end(), fault);
        if (it != m_ActiveFaults.end()) {
            it->resolved = true;
            it->resolvedAt = std::chrono::system_clock::now();
            m_History.push_back(*it);
            m_ActiveFaults.erase(it);
        }
    }

    void RegisterMonitor(const std::string& component, std::function<bool()> check, int severity = 1) {
        m_Monitors.push_back({component, check, severity});
    }

    const std::vector<FaultReport>& GetHistory() const { return m_History; }

private:
    struct Monitor {
        std::string component;
        std::function<bool()> check;
        int severity;
    };

    std::vector<Monitor> m_Monitors;
    std::vector<FaultReport> m_ActiveFaults;
    std::vector<FaultReport> m_History;
    bool m_Initialized = false;
};

} // namespace NeoEngine
