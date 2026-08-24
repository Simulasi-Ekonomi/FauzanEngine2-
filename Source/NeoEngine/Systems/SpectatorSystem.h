#pragma once
#include <vector>
#include <string>
#include <functional>

namespace NeoEngine {

struct SpectatorTarget {
    std::string playerId, playerName;
    float posX, posY, posZ;
    float rotX, rotY, rotZ;
    int health = 100;
    std::string currentAction;
    float distance;
};

class SpectatorSystem {
private:
    std::vector<SpectatorTarget> m_Targets;
    int m_CurrentTarget = -1;
    bool m_Spectating = false;
    std::string m_SpectatorId;
    std::function<void(const SpectatorTarget&)> m_OnTargetSwitch;
    
public:
    void StartSpectating(const std::string& spectatorId, const std::vector<SpectatorTarget>& targets) {
        m_SpectatorId = spectatorId;
        m_Targets = targets;
        m_Spectating = true;
        m_CurrentTarget = targets.empty() ? -1 : 0;
        if (m_CurrentTarget >= 0 && m_OnTargetSwitch) {
            m_OnTargetSwitch(m_Targets[m_CurrentTarget]);
        }
    }
    
    void StopSpectating() {
        m_Spectating = false;
        m_CurrentTarget = -1;
        m_Targets.clear();
    }
    
    void NextTarget() {
        if (!m_Spectating || m_Targets.empty()) return;
        m_CurrentTarget = (m_CurrentTarget + 1) % m_Targets.size();
        if (m_OnTargetSwitch) m_OnTargetSwitch(m_Targets[m_CurrentTarget]);
    }
    
    void PreviousTarget() {
        if (!m_Spectating || m_Targets.empty()) return;
        m_CurrentTarget = (m_CurrentTarget - 1 + m_Targets.size()) % m_Targets.size();
        if (m_OnTargetSwitch) m_OnTargetSwitch(m_Targets[m_CurrentTarget]);
    }
    
    SpectatorTarget* GetCurrentTarget() {
        if (!m_Spectating || m_CurrentTarget < 0 || m_CurrentTarget >= m_Targets.size()) return nullptr;
        return &m_Targets[m_CurrentTarget];
    }
    
    bool IsSpectating() const { return m_Spectating; }
    const std::vector<SpectatorTarget>& GetTargets() const { return m_Targets; }
    void SetOnTargetSwitch(std::function<void(const SpectatorTarget&)> cb) { m_OnTargetSwitch = cb; }
};

} // namespace NeoEngine
