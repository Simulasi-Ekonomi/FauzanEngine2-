#pragma once
// =========================================================
// NeoEngine Autonomous - FaultDetector
// Real-time fault detection and diagnostics for engine health
// Thread-safe, uses smart pointers, C++23 patterns
// =========================================================

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <optional>
#include <array>
#include <atomic>
#include <deque>

namespace Neo {
namespace Autonomous {

// Fault severity levels
enum class FaultSeverity {
    Info,       // Informational, no action needed
    Warning,    // Potential issue, monitor closely
    Error,      // Degraded operation, needs attention
    Critical,   // Severe issue, immediate action required
    Fatal       // Unrecoverable, engine must stop
};

// Fault categories
enum class FaultCategory {
    Performance,    // FPS drops, frame spikes
    Memory,         // Leaks, fragmentation, OOM
    Thermal,        // Overheating
    Rendering,      // GPU errors, shader failures
    Physics,        // Simulation instability
    Audio,          // Buffer underruns
    Network,        // Connection issues
    FileSystem,     // IO errors
    Scripting,      // Script errors
    Hardware        // Device hardware issues
};

// Individual fault record
struct Fault {
    uint64_t id = 0;
    FaultSeverity severity = FaultSeverity::Info;
    FaultCategory category = FaultCategory::Performance;
    std::string message;
    std::string source;         // subsystem that generated this
    std::string stackTrace;     // optional debug info
    std::chrono::steady_clock::time_point timestamp;
    int occurrenceCount = 1;
    bool resolved = false;

    float GetAgeSec() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<float>(now - timestamp).count();
    }
};

// Detection rule: a condition that triggers a fault
struct DetectionRule {
    std::string name;
    FaultCategory category;
    FaultSeverity severity;
    std::function<bool()> condition;    // returns true if fault detected
    std::string messageTemplate;
    float cooldownSeconds = 5.0f;       // min time between repeated detections
    std::chrono::steady_clock::time_point lastTriggered;
};

// Fault statistics for telemetry
struct FaultStats {
    int totalFaults = 0;
    int activeFaults = 0;
    int resolvedFaults = 0;
    int criticalCount = 0;
    int errorCount = 0;
    int warningCount = 0;
    float meanTimeBetweenFaults = 0.0f; // seconds
    float uptimeSeconds = 0.0f;
};

class FaultDetector {
public:
    using FaultCallback = std::function<void(const Fault&)>;

    FaultDetector()
        : m_NextFaultId(1)
        , m_StartTime(std::chrono::steady_clock::now())
    {
        RegisterDefaultRules();
    }

    ~FaultDetector() = default;

    // Non-copyable
    FaultDetector(const FaultDetector&) = delete;
    FaultDetector& operator=(const FaultDetector&) = delete;

    // Register a custom detection rule
    void RegisterRule(const std::string& name,
                      FaultCategory category,
                      FaultSeverity severity,
                      std::function<bool()> condition,
                      const std::string& message,
                      float cooldownSec = 5.0f) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        DetectionRule rule;
        rule.name = name;
        rule.category = category;
        rule.severity = severity;
        rule.condition = std::move(condition);
        rule.messageTemplate = message;
        rule.cooldownSeconds = cooldownSec;
        rule.lastTriggered = std::chrono::steady_clock::time_point{};
        m_Rules.push_back(std::move(rule));
    }

    // Set callback for new faults
    void SetFaultCallback(FaultCallback callback) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Callback = std::move(callback);
    }

    // Update metrics used by built-in rules
    void UpdateMetrics(float frameTimeMs, float cpuTemp, float memUsageMB,
                       float memBudgetMB, int drawCalls) {
        std::lock_guard<std::mutex> lock(m_MetricsMutex);
        m_FrameTimeMs = frameTimeMs;
        m_CpuTemp = cpuTemp;
        m_MemUsageMB = memUsageMB;
        m_MemBudgetMB = memBudgetMB;
        m_DrawCalls = drawCalls;

        // Track frame time history for spike detection
        m_FrameTimeHistory.push_back(frameTimeMs);
        if (m_FrameTimeHistory.size() > 120) {
            m_FrameTimeHistory.pop_front();
        }
    }

    // Run all detection rules - call each frame or periodically
    void RunDetection() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto now = std::chrono::steady_clock::now();

        for (auto& rule : m_Rules) {
            // Check cooldown
            float elapsed = std::chrono::duration<float>(now - rule.lastTriggered).count();
            if (elapsed < rule.cooldownSeconds) continue;

            // Evaluate condition
            bool triggered = false;
            try {
                triggered = rule.condition();
            } catch (...) {
                // Rule itself faulted - log but don't crash
                triggered = false;
            }

            if (triggered) {
                rule.lastTriggered = now;

                // Check if same fault already active (deduplicate)
                auto existing = FindActiveFault(rule.category, rule.messageTemplate);
                if (existing) {
                    existing->occurrenceCount++;
                    existing->timestamp = now;
                } else {
                    ReportFaultInternal(rule.severity, rule.category,
                                        rule.messageTemplate, rule.name);
                }
            }
        }
    }

    // Manually report a fault from any subsystem
    uint64_t ReportFault(FaultSeverity severity, FaultCategory category,
                         const std::string& message, const std::string& source = "") {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return ReportFaultInternal(severity, category, message, source);
    }

    // Mark a fault as resolved
    void ResolveFault(uint64_t faultId) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (auto& fault : m_ActiveFaults) {
            if (fault.id == faultId) {
                fault.resolved = true;
                m_ResolvedFaults.push_back(fault);
                break;
            }
        }
        m_ActiveFaults.erase(
            std::remove_if(m_ActiveFaults.begin(), m_ActiveFaults.end(),
                [faultId](const Fault& f) { return f.id == faultId; }),
            m_ActiveFaults.end()
        );
    }

    // Auto-resolve old faults that haven't recurred
    void AutoResolveStale(float maxAgeSec = 30.0f) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto now = std::chrono::steady_clock::now();

        auto it = std::remove_if(m_ActiveFaults.begin(), m_ActiveFaults.end(),
            [&](const Fault& f) {
                float age = std::chrono::duration<float>(now - f.timestamp).count();
                if (age > maxAgeSec) {
                    Fault resolved = f;
                    resolved.resolved = true;
                    m_ResolvedFaults.push_back(resolved);
                    return true;
                }
                return false;
            });
        m_ActiveFaults.erase(it, m_ActiveFaults.end());
    }

    // Get active faults
    std::vector<Fault> GetActiveFaults() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_ActiveFaults;
    }

    // Get active faults filtered by severity
    std::vector<Fault> GetActiveFaults(FaultSeverity minSeverity) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        std::vector<Fault> result;
        for (const auto& f : m_ActiveFaults) {
            if (static_cast<int>(f.severity) >= static_cast<int>(minSeverity)) {
                result.push_back(f);
            }
        }
        return result;
    }

    // Check if any critical/fatal faults are active
    bool HasCriticalFault() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return std::any_of(m_ActiveFaults.begin(), m_ActiveFaults.end(),
            [](const Fault& f) {
                return f.severity == FaultSeverity::Critical ||
                       f.severity == FaultSeverity::Fatal;
            });
    }

    // Get fault statistics
    FaultStats GetStats() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        FaultStats stats;
        stats.activeFaults = static_cast<int>(m_ActiveFaults.size());
        stats.resolvedFaults = static_cast<int>(m_ResolvedFaults.size());
        stats.totalFaults = stats.activeFaults + stats.resolvedFaults;

        for (const auto& f : m_ActiveFaults) {
            switch (f.severity) {
                case FaultSeverity::Critical:
                case FaultSeverity::Fatal:
                    stats.criticalCount++; break;
                case FaultSeverity::Error:
                    stats.errorCount++; break;
                case FaultSeverity::Warning:
                    stats.warningCount++; break;
                default: break;
            }
        }

        auto now = std::chrono::steady_clock::now();
        stats.uptimeSeconds = std::chrono::duration<float>(now - m_StartTime).count();
        if (stats.totalFaults > 1) {
            stats.meanTimeBetweenFaults = stats.uptimeSeconds /
                                          static_cast<float>(stats.totalFaults);
        }
        return stats;
    }

    // Get severity as string
    static const char* SeverityToString(FaultSeverity s) {
        switch (s) {
            case FaultSeverity::Info:     return "INFO";
            case FaultSeverity::Warning:  return "WARNING";
            case FaultSeverity::Error:    return "ERROR";
            case FaultSeverity::Critical: return "CRITICAL";
            case FaultSeverity::Fatal:    return "FATAL";
        }
        return "UNKNOWN";
    }

    static const char* CategoryToString(FaultCategory c) {
        switch (c) {
            case FaultCategory::Performance: return "Performance";
            case FaultCategory::Memory:      return "Memory";
            case FaultCategory::Thermal:     return "Thermal";
            case FaultCategory::Rendering:   return "Rendering";
            case FaultCategory::Physics:     return "Physics";
            case FaultCategory::Audio:       return "Audio";
            case FaultCategory::Network:     return "Network";
            case FaultCategory::FileSystem:  return "FileSystem";
            case FaultCategory::Scripting:   return "Scripting";
            case FaultCategory::Hardware:    return "Hardware";
        }
        return "Unknown";
    }

private:
    uint64_t ReportFaultInternal(FaultSeverity severity, FaultCategory category,
                                  const std::string& message, const std::string& source) {
        Fault fault;
        fault.id = m_NextFaultId++;
        fault.severity = severity;
        fault.category = category;
        fault.message = message;
        fault.source = source;
        fault.timestamp = std::chrono::steady_clock::now();
        fault.occurrenceCount = 1;
        fault.resolved = false;

        m_ActiveFaults.push_back(fault);

        // Invoke callback
        if (m_Callback) {
            m_Callback(fault);
        }

        // Cap active faults list
        if (m_ActiveFaults.size() > 500) {
            // Remove oldest info/warning faults
            auto it = std::find_if(m_ActiveFaults.begin(), m_ActiveFaults.end(),
                [](const Fault& f) {
                    return f.severity == FaultSeverity::Info ||
                           f.severity == FaultSeverity::Warning;
                });
            if (it != m_ActiveFaults.end()) {
                m_ResolvedFaults.push_back(*it);
                m_ActiveFaults.erase(it);
            }
        }

        return fault.id;
    }

    Fault* FindActiveFault(FaultCategory category, const std::string& message) {
        for (auto& f : m_ActiveFaults) {
            if (f.category == category && f.message == message && !f.resolved) {
                return &f;
            }
        }
        return nullptr;
    }

    void RegisterDefaultRules() {
        // Frame spike detection
        RegisterRule("FrameSpike", FaultCategory::Performance, FaultSeverity::Warning,
            [this]() {
                std::lock_guard<std::mutex> lock(m_MetricsMutex);
                return m_FrameTimeMs > 50.0f; // > 50ms = below 20fps
            },
            "Frame spike detected (>50ms frame time)", 2.0f);

        // Severe frame drop
        RegisterRule("SevereFrameDrop", FaultCategory::Performance, FaultSeverity::Error,
            [this]() {
                std::lock_guard<std::mutex> lock(m_MetricsMutex);
                return m_FrameTimeMs > 100.0f; // > 100ms = below 10fps
            },
            "Severe frame drop detected (>100ms frame time)", 5.0f);

        // Thermal warning
        RegisterRule("ThermalWarning", FaultCategory::Thermal, FaultSeverity::Warning,
            [this]() {
                std::lock_guard<std::mutex> lock(m_MetricsMutex);
                return m_CpuTemp > 70.0f;
            },
            "CPU temperature elevated (>70C)", 10.0f);

        // Thermal critical
        RegisterRule("ThermalCritical", FaultCategory::Thermal, FaultSeverity::Critical,
            [this]() {
                std::lock_guard<std::mutex> lock(m_MetricsMutex);
                return m_CpuTemp > 85.0f;
            },
            "CPU temperature critical (>85C)", 5.0f);

        // Memory warning
        RegisterRule("MemoryWarning", FaultCategory::Memory, FaultSeverity::Warning,
            [this]() {
                std::lock_guard<std::mutex> lock(m_MetricsMutex);
                if (m_MemBudgetMB <= 0.0f) return false;
                return (m_MemUsageMB / m_MemBudgetMB) > 0.8f;
            },
            "Memory usage exceeds 80% of budget", 15.0f);

        // Memory critical
        RegisterRule("MemoryCritical", FaultCategory::Memory, FaultSeverity::Critical,
            [this]() {
                std::lock_guard<std::mutex> lock(m_MetricsMutex);
                if (m_MemBudgetMB <= 0.0f) return false;
                return (m_MemUsageMB / m_MemBudgetMB) > 0.95f;
            },
            "Memory usage critical (>95% of budget)", 5.0f);

        // Draw call warning
        RegisterRule("DrawCallWarning", FaultCategory::Rendering, FaultSeverity::Warning,
            [this]() {
                std::lock_guard<std::mutex> lock(m_MetricsMutex);
                return m_DrawCalls > 2000;
            },
            "Excessive draw calls (>2000)", 10.0f);

        // Frame time variance (stuttering)
        RegisterRule("Stuttering", FaultCategory::Performance, FaultSeverity::Warning,
            [this]() {
                std::lock_guard<std::mutex> lock(m_MetricsMutex);
                if (m_FrameTimeHistory.size() < 30) return false;
                // Compute variance of last 30 frames
                float sum = 0.0f;
                float sumSq = 0.0f;
                size_t count = 0;
                for (auto it = m_FrameTimeHistory.rbegin();
                     it != m_FrameTimeHistory.rend() && count < 30; ++it, ++count) {
                    sum += *it;
                    sumSq += (*it) * (*it);
                }
                float mean = sum / static_cast<float>(count);
                float variance = (sumSq / static_cast<float>(count)) - (mean * mean);
                return variance > 100.0f; // high variance = stuttering
            },
            "Frame time stuttering detected (high variance)", 5.0f);
    }

    mutable std::mutex m_Mutex;
    std::mutex m_MetricsMutex;
    std::vector<Fault> m_ActiveFaults;
    std::vector<Fault> m_ResolvedFaults;
    std::vector<DetectionRule> m_Rules;
    FaultCallback m_Callback;
    uint64_t m_NextFaultId;
    std::chrono::steady_clock::time_point m_StartTime;

    // Metrics for built-in rules
    float m_FrameTimeMs = 16.67f;
    float m_CpuTemp = 45.0f;
    float m_MemUsageMB = 0.0f;
    float m_MemBudgetMB = 512.0f;
    int m_DrawCalls = 0;
    std::deque<float> m_FrameTimeHistory;
};

} // namespace Autonomous
} // namespace Neo
