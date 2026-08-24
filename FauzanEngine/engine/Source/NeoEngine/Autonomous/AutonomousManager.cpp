// =========================================================
// NeoEngine Autonomous Module - Main Integration
// Ties together DecisionEngine, FaultDetector, SelfHealer, TelemetryBus
// =========================================================

#include "DecisionEngine.h"
#include "FaultDetector.h"
#include "SelfHealer.h"
#include "TelemetryBus.h"
#include <memory>

namespace Neo {
namespace Autonomous {

// Singleton manager for autonomous subsystems
class AutonomousManager {
public:
    static AutonomousManager& Get() {
        static AutonomousManager instance;
        return instance;
    }

    void Initialize(const std::string& telemetryHost = "127.0.0.1",
                    int telemetryPort = TelemetryBus::DEFAULT_PORT) {
        if (m_Initialized) return;

        m_DecisionEngine = std::make_unique<DecisionEngine>();
        m_FaultDetector = std::make_unique<FaultDetector>();
        m_SelfHealer = std::make_unique<SelfHealer>(*m_DecisionEngine, *m_FaultDetector);
        m_TelemetryBus = std::make_unique<TelemetryBus>();

        // Start telemetry connection to Aries Python agent
        m_TelemetryBus->Start(telemetryHost, telemetryPort);

        // Set up command callback from Aries
        m_TelemetryBus->SetCommandCallback([this](const AriesCommand& cmd) {
            HandleAriesCommand(cmd);
        });

        m_Initialized = true;
    }

    void Shutdown() {
        if (!m_Initialized) return;

        m_TelemetryBus->Stop();
        m_TelemetryBus.reset();
        m_SelfHealer.reset();
        m_FaultDetector.reset();
        m_DecisionEngine.reset();

        m_Initialized = false;
    }

    // Call every frame from engine main loop
    void Tick(float deltaTimeMs, float cpuTemp, float gpuTemp,
             float cpuUsage, float gpuUsage,
             float memUsedMB, float memBudgetMB,
             float battery, int entities, int drawCalls, int triangles) {

        if (!m_Initialized) return;

        // Update metrics in decision engine
        PerformanceMetrics metrics;
        metrics.frameTimeMs = deltaTimeMs;
        metrics.cpuUsagePercent = cpuUsage;
        metrics.gpuUsagePercent = gpuUsage;
        metrics.memoryUsageMB = memUsedMB;
        metrics.memoryBudgetMB = memBudgetMB;
        metrics.temperatureCelsius = cpuTemp;
        metrics.batteryPercent = battery;
        metrics.activeEntityCount = entities;
        metrics.drawCallCount = drawCalls;
        metrics.triangleCount = triangles;
        metrics.isThermalThrottling = m_TelemetryBus->ShouldThrottle();

        m_DecisionEngine->UpdateMetrics(metrics);

        // Update fault detector
        m_FaultDetector->UpdateMetrics(deltaTimeMs, cpuTemp, memUsedMB,
                                        memBudgetMB, drawCalls);

        // Run self-healer (includes fault detection + decision evaluation)
        m_SelfHealer->Update();

        // Push telemetry to Aries agent
        auto state = m_SelfHealer->GetCurrentState();
        auto faultStats = m_FaultDetector->GetStats();
        float health = m_DecisionEngine->GetHealthScore();

        m_TelemetryBus->PushMetrics(
            deltaTimeMs, cpuTemp, gpuTemp,
            cpuUsage, gpuUsage,
            memUsedMB, memBudgetMB,
            battery, entities, drawCalls, triangles,
            state.throttleLevel, health, faultStats.activeFaults
        );

        // Process incoming Aries commands
        AriesCommand cmd;
        while (m_TelemetryBus->PopCommand(cmd)) {
            HandleAriesCommand(cmd);
        }
    }

    DecisionEngine* GetDecisionEngine() { return m_DecisionEngine.get(); }
    FaultDetector* GetFaultDetector() { return m_FaultDetector.get(); }
    SelfHealer* GetSelfHealer() { return m_SelfHealer.get(); }
    TelemetryBus* GetTelemetryBus() { return m_TelemetryBus.get(); }

private:
    AutonomousManager() : m_Initialized(false) {}
    ~AutonomousManager() { Shutdown(); }

    AutonomousManager(const AutonomousManager&) = delete;
    AutonomousManager& operator=(const AutonomousManager&) = delete;

    void HandleAriesCommand(const AriesCommand& cmd) {
        if (!m_SelfHealer) return;

        auto& state = m_SelfHealer->GetConfig();

        switch (cmd.type) {
            case AriesCommand::Type::SetThrottle:
                // Aries wants to change throttle threshold
                m_TelemetryBus->SetThrottleThreshold(static_cast<float>(cmd.intValue));
                break;

            case AriesCommand::Type::SetRenderQuality:
                // Apply through self-healer state
                break;

            case AriesCommand::Type::ForceGC:
                // Trigger garbage collection
                break;

            case AriesCommand::Type::SetTargetFPS: {
                DecisionPolicy policy = m_DecisionEngine->GetPolicy();
                policy.targetFrameTimeMs = 1000.0f / static_cast<float>(cmd.intValue);
                m_DecisionEngine->SetPolicy(policy);
                break;
            }

            default:
                break;
        }
    }

    bool m_Initialized;
    std::unique_ptr<DecisionEngine> m_DecisionEngine;
    std::unique_ptr<FaultDetector> m_FaultDetector;
    std::unique_ptr<SelfHealer> m_SelfHealer;
    std::unique_ptr<TelemetryBus> m_TelemetryBus;
};

} // namespace Autonomous
} // namespace Neo
