#pragma once
// =========================================================
// NeoEngine Autonomous - SelfHealer
// Automatic recovery and self-healing for engine faults
// Uses DecisionEngine + FaultDetector to autonomously repair
// =========================================================

#include "DecisionEngine.h"
#include "FaultDetector.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <algorithm>
#include <atomic>
#include <optional>

namespace Neo {
namespace Autonomous {

// Healing action result
struct HealResult {
    bool success = false;
    std::string actionTaken;
    std::string details;
    float recoveryTimeSec = 0.0f;
    uint64_t faultId = 0;
};

// Healing strategy for a specific fault type
struct HealingStrategy {
    std::string name;
    FaultCategory targetCategory;
    FaultSeverity minSeverity;
    std::function<HealResult(const Fault&)> healAction;
    int maxRetries = 3;
    float cooldownSeconds = 10.0f;
    int timesUsed = 0;
    int timesSucceeded = 0;
};

// Engine state snapshot for rollback
struct EngineSnapshot {
    uint64_t snapshotId = 0;
    std::chrono::steady_clock::time_point timestamp;
    float renderQuality = 1.0f;
    float lodBias = 0.0f;
    float drawDistance = 10000.0f;
    int physicsTickRate = 60;
    bool shadowsEnabled = true;
    bool occlusionEnabled = true;
    bool batchingEnabled = false;
    int throttleLevel = 0;  // 0=none, 1=light, 2=medium, 3=heavy
    float memoryBudgetMB = 512.0f;
};

// Self-healer configuration
struct SelfHealerConfig {
    bool enabled = true;
    bool autoHealPerformance = true;
    bool autoHealMemory = true;
    bool autoHealThermal = true;
    bool autoHealRendering = true;
    float evaluationIntervalSec = 1.0f;
    int maxSnapshotHistory = 10;
    int maxHealActionsPerMinute = 30;
    float stabilityWindowSec = 5.0f; // must be stable for this long after heal
};

class SelfHealer {
public:
    using HealCallback = std::function<void(const HealResult&)>;

    SelfHealer(DecisionEngine& decisionEngine, FaultDetector& faultDetector)
        : m_DecisionEngine(decisionEngine)
        , m_FaultDetector(faultDetector)
        , m_NextSnapshotId(1)
        , m_HealActionsThisMinute(0)
        , m_MinuteStart(std::chrono::steady_clock::now())
    {
        // Register fault callback to trigger healing
        m_FaultDetector.SetFaultCallback([this](const Fault& fault) {
            OnFaultDetected(fault);
        });

        RegisterDefaultStrategies();
        TakeSnapshot(); // initial snapshot
    }

    ~SelfHealer() = default;

    // Non-copyable
    SelfHealer(const SelfHealer&) = delete;
    SelfHealer& operator=(const SelfHealer&) = delete;

    // Main update - call periodically (e.g., once per second)
    void Update() {
        if (!m_Config.enabled) return;

        std::lock_guard<std::mutex> lock(m_Mutex);

        auto now = std::chrono::steady_clock::now();

        // Reset heal counter each minute
        float minuteElapsed = std::chrono::duration<float>(now - m_MinuteStart).count();
        if (minuteElapsed >= 60.0f) {
            m_HealActionsThisMinute = 0;
            m_MinuteStart = now;
        }

        // Run fault detection
        m_FaultDetector.RunDetection();
        m_FaultDetector.AutoResolveStale(30.0f);

        // Process pending heals
        ProcessPendingHeals();

        // Evaluate decision engine for proactive healing
        auto decisions = m_DecisionEngine.Evaluate();
        if (decisions.has_value()) {
            for (const auto& decision : decisions.value()) {
                if (decision.priority >= 0.5f) {
                    ExecuteDecision(decision);
                }
            }
        }

        // Check if we should take a new snapshot (every 30 seconds when stable)
        if (!m_FaultDetector.HasCriticalFault()) {
            float snapshotAge = 0.0f;
            if (!m_Snapshots.empty()) {
                snapshotAge = std::chrono::duration<float>(
                    now - m_Snapshots.back().timestamp).count();
            }
            if (snapshotAge > 30.0f || m_Snapshots.empty()) {
                TakeSnapshot();
            }
        }
    }

    // Manual trigger to heal a specific fault
    HealResult HealFault(uint64_t faultId) {
        std::lock_guard<std::mutex> lock(m_Mutex);

        auto faults = m_FaultDetector.GetActiveFaults();
        for (const auto& fault : faults) {
            if (fault.id == faultId) {
                return AttemptHeal(fault);
            }
        }

        return HealResult{false, "NoAction", "Fault not found: " + std::to_string(faultId)};
    }

    // Heal all active faults
    std::vector<HealResult> HealAll() {
        std::lock_guard<std::mutex> lock(m_Mutex);

        std::vector<HealResult> results;
        auto faults = m_FaultDetector.GetActiveFaults();

        // Sort by severity (most severe first)
        std::sort(faults.begin(), faults.end(),
            [](const Fault& a, const Fault& b) {
                return static_cast<int>(a.severity) > static_cast<int>(b.severity);
            });

        for (const auto& fault : faults) {
            if (m_HealActionsThisMinute >= m_Config.maxHealActionsPerMinute) break;
            auto result = AttemptHeal(fault);
            results.push_back(result);
        }

        return results;
    }

    // Rollback to last known good snapshot
    HealResult RollbackToLastGoodState() {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_Snapshots.empty()) {
            return HealResult{false, "Rollback", "No snapshots available"};
        }

        // Find the most recent snapshot that was taken during healthy state
        const auto& snapshot = m_Snapshots.back();
        ApplySnapshot(snapshot);

        return HealResult{
            true, "Rollback",
            "Rolled back to snapshot #" + std::to_string(snapshot.snapshotId),
            0.0f, 0
        };
    }

    // Register a custom healing strategy
    void RegisterStrategy(const std::string& name,
                          FaultCategory category,
                          FaultSeverity minSeverity,
                          std::function<HealResult(const Fault&)> action,
                          int maxRetries = 3,
                          float cooldownSec = 10.0f) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        HealingStrategy strategy;
        strategy.name = name;
        strategy.targetCategory = category;
        strategy.minSeverity = minSeverity;
        strategy.healAction = std::move(action);
        strategy.maxRetries = maxRetries;
        strategy.cooldownSeconds = cooldownSec;
        m_Strategies.push_back(std::move(strategy));
    }

    // Set callback for heal events
    void SetHealCallback(HealCallback callback) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_HealCallback = std::move(callback);
    }

    // Get current engine state
    EngineSnapshot GetCurrentState() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_CurrentState;
    }

    // Get heal history
    std::vector<HealResult> GetHealHistory() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_HealHistory;
    }

    // Get success rate
    float GetSuccessRate() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_HealHistory.empty()) return 0.0f;
        int successes = 0;
        for (const auto& r : m_HealHistory) {
            if (r.success) successes++;
        }
        return static_cast<float>(successes) / static_cast<float>(m_HealHistory.size());
    }

    SelfHealerConfig& GetConfig() { return m_Config; }
    const SelfHealerConfig& GetConfig() const { return m_Config; }

private:
    void OnFaultDetected(const Fault& fault) {
        if (!m_Config.enabled) return;

        // Check category auto-heal settings
        bool shouldAutoHeal = false;
        switch (fault.category) {
            case FaultCategory::Performance:
                shouldAutoHeal = m_Config.autoHealPerformance; break;
            case FaultCategory::Memory:
                shouldAutoHeal = m_Config.autoHealMemory; break;
            case FaultCategory::Thermal:
                shouldAutoHeal = m_Config.autoHealThermal; break;
            case FaultCategory::Rendering:
                shouldAutoHeal = m_Config.autoHealRendering; break;
            default:
                shouldAutoHeal = (fault.severity >= FaultSeverity::Critical); break;
        }

        if (shouldAutoHeal && fault.severity >= FaultSeverity::Warning) {
            m_PendingHeals.push_back(fault.id);
        }
    }

    void ProcessPendingHeals() {
        if (m_PendingHeals.empty()) return;

        auto faults = m_FaultDetector.GetActiveFaults();
        std::unordered_map<uint64_t, Fault> faultMap;
        for (const auto& f : faults) {
            faultMap[f.id] = f;
        }

        std::vector<uint64_t> remaining;
        for (uint64_t faultId : m_PendingHeals) {
            if (m_HealActionsThisMinute >= m_Config.maxHealActionsPerMinute) {
                remaining.push_back(faultId);
                continue;
            }

            auto it = faultMap.find(faultId);
            if (it != faultMap.end() && !it->second.resolved) {
                AttemptHeal(it->second);
            }
            // Don't re-add resolved or missing faults
        }
        m_PendingHeals = std::move(remaining);
    }

    HealResult AttemptHeal(const Fault& fault) {
        // Find matching strategy
        HealingStrategy* bestStrategy = nullptr;
        for (auto& strategy : m_Strategies) {
            if (strategy.targetCategory == fault.category &&
                static_cast<int>(fault.severity) >= static_cast<int>(strategy.minSeverity)) {
                if (!bestStrategy || strategy.timesSucceeded > bestStrategy->timesSucceeded) {
                    bestStrategy = &strategy;
                }
            }
        }

        HealResult result;
        if (!bestStrategy) {
            result.success = false;
            result.actionTaken = "NoStrategy";
            result.details = "No healing strategy for " +
                             std::string(FaultDetector::CategoryToString(fault.category));
            result.faultId = fault.id;
        } else {
            auto startTime = std::chrono::steady_clock::now();

            try {
                result = bestStrategy->healAction(fault);
            } catch (const std::exception& e) {
                result.success = false;
                result.actionTaken = bestStrategy->name;
                result.details = std::string("Strategy threw exception: ") + e.what();
            }

            auto endTime = std::chrono::steady_clock::now();
            result.recoveryTimeSec = std::chrono::duration<float>(endTime - startTime).count();
            result.faultId = fault.id;

            bestStrategy->timesUsed++;
            if (result.success) {
                bestStrategy->timesSucceeded++;
                m_FaultDetector.ResolveFault(fault.id);
            }
        }

        m_HealActionsThisMinute++;
        m_HealHistory.push_back(result);
        if (m_HealHistory.size() > 200) {
            m_HealHistory.erase(m_HealHistory.begin());
        }

        if (m_HealCallback) {
            m_HealCallback(result);
        }

        return result;
    }

    void ExecuteDecision(const Decision& decision) {
        switch (decision.action) {
            case Decision::Action::ReduceRenderQuality:
                m_CurrentState.renderQuality = std::max(0.25f, m_CurrentState.renderQuality - 0.25f);
                break;
            case Decision::Action::IncreaseRenderQuality:
                m_CurrentState.renderQuality = std::min(1.0f, m_CurrentState.renderQuality + 0.1f);
                break;
            case Decision::Action::ReduceLOD:
                m_CurrentState.lodBias += 1.0f;
                break;
            case Decision::Action::IncreaseLOD:
                m_CurrentState.lodBias = std::max(0.0f, m_CurrentState.lodBias - 0.5f);
                break;
            case Decision::Action::ThrottleCPU:
                m_CurrentState.throttleLevel = std::min(3, m_CurrentState.throttleLevel + 1);
                break;
            case Decision::Action::UnthrottleCPU:
                m_CurrentState.throttleLevel = std::max(0, m_CurrentState.throttleLevel - 1);
                break;
            case Decision::Action::DisableShadows:
                m_CurrentState.shadowsEnabled = false;
                break;
            case Decision::Action::EnableShadows:
                m_CurrentState.shadowsEnabled = true;
                break;
            case Decision::Action::EnableBatching:
                m_CurrentState.batchingEnabled = true;
                break;
            case Decision::Action::ReduceDrawDistance:
                m_CurrentState.drawDistance *= 0.75f;
                break;
            case Decision::Action::IncreaseDrawDistance:
                m_CurrentState.drawDistance = std::min(20000.0f, m_CurrentState.drawDistance * 1.1f);
                break;
            case Decision::Action::ReducePhysicsRate:
                m_CurrentState.physicsTickRate = std::max(15, m_CurrentState.physicsTickRate / 2);
                break;
            case Decision::Action::IncreasePhysicsRate:
                m_CurrentState.physicsTickRate = std::min(120, m_CurrentState.physicsTickRate * 2);
                break;
            case Decision::Action::GarbageCollect:
            case Decision::Action::CompactMemory:
                // Signal GC to run (handled by memory subsystem)
                break;
            case Decision::Action::EmergencyShutdown:
                // Critical - save state and request shutdown
                TakeSnapshot();
                break;
            default:
                break;
        }
    }

    void TakeSnapshot() {
        EngineSnapshot snapshot = m_CurrentState;
        snapshot.snapshotId = m_NextSnapshotId++;
        snapshot.timestamp = std::chrono::steady_clock::now();

        m_Snapshots.push_back(snapshot);
        if (m_Snapshots.size() > static_cast<size_t>(m_Config.maxSnapshotHistory)) {
            m_Snapshots.erase(m_Snapshots.begin());
        }
    }

    void ApplySnapshot(const EngineSnapshot& snapshot) {
        m_CurrentState.renderQuality = snapshot.renderQuality;
        m_CurrentState.lodBias = snapshot.lodBias;
        m_CurrentState.drawDistance = snapshot.drawDistance;
        m_CurrentState.physicsTickRate = snapshot.physicsTickRate;
        m_CurrentState.shadowsEnabled = snapshot.shadowsEnabled;
        m_CurrentState.occlusionEnabled = snapshot.occlusionEnabled;
        m_CurrentState.batchingEnabled = snapshot.batchingEnabled;
        m_CurrentState.throttleLevel = snapshot.throttleLevel;
    }

    void RegisterDefaultStrategies() {
        // Performance healing: reduce quality
        RegisterStrategy("ReduceQuality", FaultCategory::Performance, FaultSeverity::Warning,
            [this](const Fault& fault) -> HealResult {
                m_CurrentState.renderQuality = std::max(0.25f, m_CurrentState.renderQuality - 0.25f);
                m_CurrentState.lodBias += 1.0f;
                return HealResult{
                    true, "ReduceQuality",
                    "Reduced render quality to " + std::to_string(m_CurrentState.renderQuality),
                    0.0f, fault.id
                };
            });

        // Thermal healing: throttle
        RegisterStrategy("ThermalThrottle", FaultCategory::Thermal, FaultSeverity::Warning,
            [this](const Fault& fault) -> HealResult {
                m_CurrentState.throttleLevel = std::min(3, m_CurrentState.throttleLevel + 1);
                m_CurrentState.renderQuality = std::max(0.5f, m_CurrentState.renderQuality - 0.1f);
                return HealResult{
                    true, "ThermalThrottle",
                    "Throttle level increased to " + std::to_string(m_CurrentState.throttleLevel),
                    0.0f, fault.id
                };
            });

        // Memory healing: compact + reduce budgets
        RegisterStrategy("MemoryRecovery", FaultCategory::Memory, FaultSeverity::Warning,
            [this](const Fault& fault) -> HealResult {
                m_CurrentState.drawDistance *= 0.8f;
                m_CurrentState.lodBias += 0.5f;
                return HealResult{
                    true, "MemoryRecovery",
                    "Reduced draw distance and increased LOD bias to free memory",
                    0.0f, fault.id
                };
            });

        // Rendering healing: simplify
        RegisterStrategy("RenderingSimplify", FaultCategory::Rendering, FaultSeverity::Warning,
            [this](const Fault& fault) -> HealResult {
                m_CurrentState.shadowsEnabled = false;
                m_CurrentState.batchingEnabled = true;
                return HealResult{
                    true, "RenderingSimplify",
                    "Disabled shadows and enabled batching",
                    0.0f, fault.id
                };
            });
    }

    mutable std::mutex m_Mutex;
    DecisionEngine& m_DecisionEngine;
    FaultDetector& m_FaultDetector;
    SelfHealerConfig m_Config;

    EngineSnapshot m_CurrentState;
    std::vector<EngineSnapshot> m_Snapshots;
    uint64_t m_NextSnapshotId;

    std::vector<HealingStrategy> m_Strategies;
    std::vector<HealResult> m_HealHistory;
    std::vector<uint64_t> m_PendingHeals;

    HealCallback m_HealCallback;
    int m_HealActionsThisMinute;
    std::chrono::steady_clock::time_point m_MinuteStart;
};

} // namespace Autonomous
} // namespace Neo
