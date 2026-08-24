#pragma once
#include <memory>
namespace NeoEngine {
class DecisionEngine;
class FaultDetector;
class SelfHealer;
class TelemetryBus;
class AutonomousManager {
public:
    AutonomousManager();
    ~AutonomousManager();
    void Initialize();
    void Update(float dt);
private:
    std::unique_ptr<DecisionEngine> m_DecisionEngine;
    std::unique_ptr<FaultDetector> m_FaultDetector;
    std::unique_ptr<SelfHealer> m_SelfHealer;
    std::unique_ptr<TelemetryBus> m_TelemetryBus;
};
}
