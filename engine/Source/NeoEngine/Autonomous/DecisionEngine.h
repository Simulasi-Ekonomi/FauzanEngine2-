#pragma once
// =========================================================
// NeoEngine Autonomous - DecisionEngine
// AI-driven decision making for engine self-management
// Uses C++23 std::expected for error handling
// =========================================================

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <optional>
#include <variant>
#include <array>

namespace Neo {
namespace Autonomous {

// C++23-style expected (polyfill for compilers without full C++23)
template<typename T, typename E>
class Expected {
public:
    Expected(T value) : m_Value(std::move(value)), m_HasValue(true) {}
    static Expected Error(E error) {
        Expected e;
        e.m_Error = std::move(error);
        e.m_HasValue = false;
        return e;
    }
    bool has_value() const { return m_HasValue; }
    const T& value() const { return m_Value; }
    T& value() { return m_Value; }
    const E& error() const { return m_Error; }
    explicit operator bool() const { return m_HasValue; }
private:
    Expected() : m_HasValue(false) {}
    T m_Value{};
    E m_Error{};
    bool m_HasValue;
};

// Error types for decision engine
struct DecisionError {
    enum class Code {
        InsufficientData,
        InvalidState,
        ThresholdExceeded,
        PolicyViolation,
        TimeoutExpired,
        ResourceUnavailable
    };
    Code code;
    std::string message;
};

// Performance metrics fed into decision engine
struct PerformanceMetrics {
    float frameTimeMs = 16.67f;
    float cpuUsagePercent = 0.0f;
    float gpuUsagePercent = 0.0f;
    float memoryUsageMB = 0.0f;
    float memoryBudgetMB = 512.0f;
    float temperatureCelsius = 45.0f;
    int activeEntityCount = 0;
    int drawCallCount = 0;
    int triangleCount = 0;
    float batteryPercent = 100.0f;
    bool isThermalThrottling = false;
};

// Decision output
struct Decision {
    enum class Action {
        NoChange,
        ReduceRenderQuality,
        IncreaseRenderQuality,
        ReduceLOD,
        IncreaseLOD,
        EnableOcclusion,
        DisableOcclusion,
        ReducePhysicsRate,
        IncreasePhysicsRate,
        GarbageCollect,
        ThrottleCPU,
        UnthrottleCPU,
        ReduceDrawDistance,
        IncreaseDrawDistance,
        EnableBatching,
        DisableShadows,
        EnableShadows,
        CompactMemory,
        EmergencyShutdown
    };

    Action action = Action::NoChange;
    float priority = 0.0f;  // 0.0 (low) to 1.0 (critical)
    std::string reason;
    float confidence = 0.0f; // 0.0 to 1.0
};

// Policy defines thresholds and weights for decision making
struct DecisionPolicy {
    float targetFrameTimeMs = 16.67f;   // 60 FPS target
    float maxCpuUsage = 85.0f;          // percent
    float maxGpuUsage = 90.0f;
    float maxTemperature = 75.0f;       // celsius
    float criticalTemperature = 85.0f;
    float memoryWarningPercent = 80.0f;
    float memoryCriticalPercent = 95.0f;
    float minBatteryPercent = 10.0f;
    int maxDrawCalls = 2000;
    int maxTriangles = 2000000;

    // Weights for multi-criteria decision
    float frameTimeWeight = 0.3f;
    float temperatureWeight = 0.25f;
    float memoryWeight = 0.2f;
    float cpuWeight = 0.15f;
    float batteryWeight = 0.1f;
};

// History entry for learning from past decisions
struct DecisionHistoryEntry {
    Decision decision;
    PerformanceMetrics metricsBefore;
    PerformanceMetrics metricsAfter;
    std::chrono::steady_clock::time_point timestamp;
    bool wasEffective = false;
};

class DecisionEngine {
public:
    DecisionEngine()
        : m_Policy()
        , m_HistoryMaxSize(100)
    {
        m_MetricsHistory.reserve(60); // 1 second of history at 60fps
    }

    ~DecisionEngine() = default;

    // Non-copyable, movable
    DecisionEngine(const DecisionEngine&) = delete;
    DecisionEngine& operator=(const DecisionEngine&) = delete;
    DecisionEngine(DecisionEngine&&) = default;
    DecisionEngine& operator=(DecisionEngine&&) = default;

    // Feed metrics into the engine
    void UpdateMetrics(const PerformanceMetrics& metrics) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_CurrentMetrics = metrics;
        m_MetricsHistory.push_back(metrics);
        if (m_MetricsHistory.size() > 300) { // keep ~5 seconds at 60fps
            m_MetricsHistory.erase(m_MetricsHistory.begin());
        }
    }

    // Evaluate current state and produce decisions
    Expected<std::vector<Decision>, DecisionError> Evaluate() {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_MetricsHistory.size() < 5) {
            return Expected<std::vector<Decision>, DecisionError>::Error(
                {DecisionError::Code::InsufficientData,
                 "Need at least 5 frames of metrics data"}
            );
        }

        std::vector<Decision> decisions;

        // Evaluate each concern
        auto tempDecision = EvaluateTemperature();
        if (tempDecision.has_value()) decisions.push_back(tempDecision.value());

        auto frameDecision = EvaluateFrameTime();
        if (frameDecision.has_value()) decisions.push_back(frameDecision.value());

        auto memDecision = EvaluateMemory();
        if (memDecision.has_value()) decisions.push_back(memDecision.value());

        auto cpuDecision = EvaluateCPU();
        if (cpuDecision.has_value()) decisions.push_back(cpuDecision.value());

        auto battDecision = EvaluateBattery();
        if (battDecision.has_value()) decisions.push_back(battDecision.value());

        auto drawDecision = EvaluateDrawCalls();
        if (drawDecision.has_value()) decisions.push_back(drawDecision.value());

        // Sort by priority (highest first)
        std::sort(decisions.begin(), decisions.end(),
            [](const Decision& a, const Decision& b) {
                return a.priority > b.priority;
            });

        return decisions;
    }

    // Record outcome of a decision for future learning
    void RecordOutcome(const Decision& decision,
                       const PerformanceMetrics& before,
                       const PerformanceMetrics& after) {
        std::lock_guard<std::mutex> lock(m_Mutex);

        DecisionHistoryEntry entry;
        entry.decision = decision;
        entry.metricsBefore = before;
        entry.metricsAfter = after;
        entry.timestamp = std::chrono::steady_clock::now();

        // Determine effectiveness: did the action improve the targeted metric?
        entry.wasEffective = EvaluateEffectiveness(decision, before, after);

        m_DecisionHistory.push_back(std::move(entry));
        if (m_DecisionHistory.size() > m_HistoryMaxSize) {
            m_DecisionHistory.erase(m_DecisionHistory.begin());
        }
    }

    // Get effectiveness rate of past decisions
    float GetEffectivenessRate() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_DecisionHistory.empty()) return 0.0f;

        int effective = 0;
        for (const auto& entry : m_DecisionHistory) {
            if (entry.wasEffective) effective++;
        }
        return static_cast<float>(effective) / static_cast<float>(m_DecisionHistory.size());
    }

    // Get computed health score (0.0 = critical, 1.0 = perfect)
    float GetHealthScore() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return ComputeHealthScore(m_CurrentMetrics);
    }

    void SetPolicy(const DecisionPolicy& policy) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Policy = policy;
    }

    const DecisionPolicy& GetPolicy() const { return m_Policy; }
    const PerformanceMetrics& GetCurrentMetrics() const { return m_CurrentMetrics; }

private:
    float ComputeHealthScore(const PerformanceMetrics& m) const {
        float frameScore = std::clamp(1.0f - (m.frameTimeMs - m_Policy.targetFrameTimeMs) /
                                      m_Policy.targetFrameTimeMs, 0.0f, 1.0f);
        float tempScore = std::clamp(1.0f - (m.temperatureCelsius - 40.0f) /
                                     (m_Policy.criticalTemperature - 40.0f), 0.0f, 1.0f);
        float memScore = std::clamp(1.0f - m.memoryUsageMB / m.memoryBudgetMB, 0.0f, 1.0f);
        float cpuScore = std::clamp(1.0f - m.cpuUsagePercent / 100.0f, 0.0f, 1.0f);
        float battScore = std::clamp(m.batteryPercent / 100.0f, 0.0f, 1.0f);

        return frameScore * m_Policy.frameTimeWeight +
               tempScore * m_Policy.temperatureWeight +
               memScore * m_Policy.memoryWeight +
               cpuScore * m_Policy.cpuWeight +
               battScore * m_Policy.batteryWeight;
    }

    std::optional<Decision> EvaluateTemperature() {
        float avgTemp = ComputeAverageMetric(
            [](const PerformanceMetrics& m) { return m.temperatureCelsius; });

        if (avgTemp >= m_Policy.criticalTemperature) {
            return Decision{
                Decision::Action::EmergencyShutdown,
                1.0f,
                "Critical temperature reached: " + std::to_string(static_cast<int>(avgTemp)) + "C",
                0.99f
            };
        }
        if (avgTemp >= m_Policy.maxTemperature) {
            float severity = (avgTemp - m_Policy.maxTemperature) /
                             (m_Policy.criticalTemperature - m_Policy.maxTemperature);
            return Decision{
                Decision::Action::ThrottleCPU,
                0.7f + severity * 0.2f,
                "High temperature: " + std::to_string(static_cast<int>(avgTemp)) + "C",
                0.85f
            };
        }
        if (avgTemp < m_Policy.maxTemperature * 0.8f && m_CurrentMetrics.isThermalThrottling) {
            return Decision{
                Decision::Action::UnthrottleCPU,
                0.3f,
                "Temperature normalized, removing throttle",
                0.75f
            };
        }
        return std::nullopt;
    }

    std::optional<Decision> EvaluateFrameTime() {
        float avgFrameTime = ComputeAverageMetric(
            [](const PerformanceMetrics& m) { return m.frameTimeMs; });

        if (avgFrameTime > m_Policy.targetFrameTimeMs * 2.0f) {
            return Decision{
                Decision::Action::ReduceRenderQuality,
                0.8f,
                "Frame time severely exceeded: " + std::to_string(static_cast<int>(avgFrameTime)) + "ms",
                0.9f
            };
        }
        if (avgFrameTime > m_Policy.targetFrameTimeMs * 1.3f) {
            return Decision{
                Decision::Action::ReduceLOD,
                0.5f,
                "Frame time elevated: " + std::to_string(static_cast<int>(avgFrameTime)) + "ms",
                0.8f
            };
        }
        if (avgFrameTime < m_Policy.targetFrameTimeMs * 0.7f) {
            return Decision{
                Decision::Action::IncreaseRenderQuality,
                0.2f,
                "Frame budget available for quality increase",
                0.6f
            };
        }
        return std::nullopt;
    }

    std::optional<Decision> EvaluateMemory() {
        float usagePercent = (m_CurrentMetrics.memoryUsageMB / m_CurrentMetrics.memoryBudgetMB) * 100.0f;

        if (usagePercent >= m_Policy.memoryCriticalPercent) {
            return Decision{
                Decision::Action::GarbageCollect,
                0.9f,
                "Critical memory usage: " + std::to_string(static_cast<int>(usagePercent)) + "%",
                0.95f
            };
        }
        if (usagePercent >= m_Policy.memoryWarningPercent) {
            return Decision{
                Decision::Action::CompactMemory,
                0.5f,
                "High memory usage: " + std::to_string(static_cast<int>(usagePercent)) + "%",
                0.8f
            };
        }
        return std::nullopt;
    }

    std::optional<Decision> EvaluateCPU() {
        float avgCPU = ComputeAverageMetric(
            [](const PerformanceMetrics& m) { return m.cpuUsagePercent; });

        if (avgCPU > m_Policy.maxCpuUsage) {
            return Decision{
                Decision::Action::ReducePhysicsRate,
                0.6f,
                "CPU usage high: " + std::to_string(static_cast<int>(avgCPU)) + "%",
                0.75f
            };
        }
        return std::nullopt;
    }

    std::optional<Decision> EvaluateBattery() {
        if (m_CurrentMetrics.batteryPercent < m_Policy.minBatteryPercent) {
            return Decision{
                Decision::Action::DisableShadows,
                0.4f,
                "Low battery: " + std::to_string(static_cast<int>(m_CurrentMetrics.batteryPercent)) + "%",
                0.7f
            };
        }
        return std::nullopt;
    }

    std::optional<Decision> EvaluateDrawCalls() {
        if (m_CurrentMetrics.drawCallCount > m_Policy.maxDrawCalls) {
            return Decision{
                Decision::Action::EnableBatching,
                0.4f,
                "Draw calls exceeded limit: " + std::to_string(m_CurrentMetrics.drawCallCount),
                0.85f
            };
        }
        return std::nullopt;
    }

    bool EvaluateEffectiveness(const Decision& decision,
                               const PerformanceMetrics& before,
                               const PerformanceMetrics& after) {
        switch (decision.action) {
            case Decision::Action::ReduceRenderQuality:
            case Decision::Action::ReduceLOD:
                return after.frameTimeMs < before.frameTimeMs;
            case Decision::Action::ThrottleCPU:
                return after.temperatureCelsius < before.temperatureCelsius;
            case Decision::Action::GarbageCollect:
            case Decision::Action::CompactMemory:
                return after.memoryUsageMB < before.memoryUsageMB;
            case Decision::Action::ReducePhysicsRate:
                return after.cpuUsagePercent < before.cpuUsagePercent;
            case Decision::Action::EnableBatching:
                return after.drawCallCount < before.drawCallCount;
            default:
                return true;
        }
    }

    float ComputeAverageMetric(std::function<float(const PerformanceMetrics&)> extractor) {
        if (m_MetricsHistory.empty()) return 0.0f;
        size_t count = std::min(m_MetricsHistory.size(), static_cast<size_t>(30));
        float sum = 0.0f;
        for (size_t i = m_MetricsHistory.size() - count; i < m_MetricsHistory.size(); i++) {
            sum += extractor(m_MetricsHistory[i]);
        }
        return sum / static_cast<float>(count);
    }

    mutable std::mutex m_Mutex;
    DecisionPolicy m_Policy;
    PerformanceMetrics m_CurrentMetrics;
    std::vector<PerformanceMetrics> m_MetricsHistory;
    std::vector<DecisionHistoryEntry> m_DecisionHistory;
    size_t m_HistoryMaxSize;
};

} // namespace Autonomous
} // namespace Neo
