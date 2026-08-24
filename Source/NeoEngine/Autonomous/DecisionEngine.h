#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <memory>
#include <any>

namespace NeoEngine {

struct TelemetrySnapshot; // Forward-declared

// Representasi keputusan yang diambil
struct Decision {
    std::string id;
    std::string ruleName;
    std::string description;
    std::chrono::system_clock::time_point timestamp;
    bool executed = false;
    std::any result;
};

class DecisionEngine {
public:
    DecisionEngine() = default;
    ~DecisionEngine() = default;

    void Initialize() { m_Initialized = true; }

    // Evaluasi snapshot telemetri dan jalankan aturan yang cocok
    void Evaluate(const TelemetrySnapshot& snapshot) {
        if (!m_Initialized) return;
        for (auto& rule : m_Rules) {
            if (rule.condition && rule.condition(snapshot)) {
                Decision d;
                d.id = "dec_" + std::to_string(m_Log.size());
                d.ruleName = rule.name;
                d.timestamp = std::chrono::system_clock::now();
                if (rule.action) rule.action();
                d.executed = true;
                m_Log.push_back(d);
            }
        }
    }

    // Mendaftarkan aturan pengambilan keputusan
    void RegisterRule(
        const std::string& name,
        std::function<bool(const TelemetrySnapshot&)> condition,
        std::function<void()> action,
        int priority = 0) {
        m_Rules.push_back({name, condition, action, priority});
    }

    // Mendapatkan log keputusan yang telah dibuat
    std::vector<Decision> GetDecisionLog() const { return m_Log; }
    bool IsReady() const { return m_Initialized; }

private:
    struct Rule {
        std::string name;
        std::function<bool(const TelemetrySnapshot&)> condition;
        std::function<void()> action;
        int priority = 0;
    };

    std::vector<Rule> m_Rules;
    std::vector<Decision> m_Log;
    bool m_Initialized = false;
};

} // namespace NeoEngine
