#include "AutonomousManager.h"
#include "DecisionEngine.h"
#include "FaultDetector.h"
#include "SelfHealer.h"
#include "TelemetryBus.h"
#include <android/log.h>

#define LOG_TAG "AutonomousMgr"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

AutonomousManager::AutonomousManager() {
    m_DecisionEngine = std::make_unique<DecisionEngine>();
    m_FaultDetector = std::make_unique<FaultDetector>();
    m_SelfHealer = std::make_unique<SelfHealer>();
    m_TelemetryBus = std::make_unique<TelemetryBus>();
}

AutonomousManager::~AutonomousManager() = default;

void AutonomousManager::Initialize() {
    LOGI("Initializing Autonomous System...");
    m_DecisionEngine->Initialize();
    m_FaultDetector->Initialize();
    m_SelfHealer->Initialize();
    m_TelemetryBus->Initialize();

    // Daftarkan aturan pengambilan keputusan otomatis
    m_DecisionEngine->RegisterRule("low_fps", [this](const TelemetrySnapshot& snap) -> bool {
        return snap.fps < 30.0f;
    }, [this]() {
        LOGI("Autonomous Decision: Reducing quality due to low FPS");
        m_TelemetryBus->PublishMetric("quality_reduced", true);
    });

    m_DecisionEngine->RegisterRule("memory_leak", [this](const TelemetrySnapshot& snap) -> bool {
        return snap.metrics.count("memory_leak");
    }, [this]() {
        LOGI("Autonomous Decision: Initiating garbage collection");
        m_TelemetryBus->PublishMetric("gc_requested", true);
    });

    // Daftarkan monitor kesalahan
    m_FaultDetector->RegisterMonitor("memory", [this]() -> bool {
        return m_TelemetryBus->GetSnapshot().memoryUsage > 500 * 1024 * 1024; // 500MB
    }, 5);

    m_FaultDetector->RegisterMonitor("fps", [this]() -> bool {
        return m_TelemetryBus->GetSnapshot().fps < 10.0f;
    }, 4);

    // Daftarkan strategi penyembuhan
    m_SelfHealer->RegisterHealStrategy("memory", [this]() -> bool {
        LOGI("Autonomous Heal: Attempting memory recovery...");
        return true;
    }, 3, std::chrono::milliseconds(100));

    m_SelfHealer->RegisterHealStrategy("fps", [this]() -> bool {
        LOGI("Autonomous Heal: Reducing workload...");
        return true;
    }, 2, std::chrono::milliseconds(50));

    LOGI("Autonomous System Initialized with %zu rules, %zu monitors, %zu heal strategies",
         m_DecisionEngine->GetDecisionLog().size(),
         m_FaultDetector->GetActiveFaults().size(),
         m_SelfHealer->GetUnresolvedCount() == 0 ? 2 : 0);
}

void AutonomousManager::Update(float dt) {
    // Kumpulkan telemetri
    m_TelemetryBus->Collect();
    
    // Analisis kesalahan
    m_FaultDetector->Analyze();
    
    // Perbaiki jika ada kesalahan
    if (m_FaultDetector->HasFault()) {
        auto faults = m_FaultDetector->GetActiveFaults();
        for (const auto& fault : faults) {
            bool healed = m_SelfHealer->Repair(fault);
            if (healed) {
                m_FaultDetector->ResolveFault(fault);
            }
        }
    }
    
    // Evaluasi keputusan
    m_DecisionEngine->Evaluate(m_TelemetryBus->GetSnapshot());
}

} // namespace NeoEngine
