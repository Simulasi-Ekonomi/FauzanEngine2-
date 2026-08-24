#pragma once
#include <string>
#include <chrono>
#include <functional>

namespace NeoEngine {

class EnergySystem {
private:
    int m_MaxEnergy = 100;
    int m_CurrentEnergy = 100;
    int m_EnergyPerMinute = 1;
    int m_RefillCost = 50; // gems
    std::chrono::system_clock::time_point m_LastRefillTime;
    std::function<void(int)> m_OnEnergyChanged;

public:
    EnergySystem() { m_LastRefillTime = std::chrono::system_clock::now(); }

    void Update() {
        auto now = std::chrono::system_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::minutes>(now - m_LastRefillTime).count();
        int energyGained = diff * m_EnergyPerMinute;
        if (energyGained > 0) {
            m_CurrentEnergy = std::min(m_MaxEnergy, m_CurrentEnergy + energyGained);
            m_LastRefillTime = now;
            if (m_OnEnergyChanged) m_OnEnergyChanged(m_CurrentEnergy);
        }
    }

    bool ConsumeEnergy(int amount) {
        if (m_CurrentEnergy < amount) return false;
        m_CurrentEnergy -= amount;
        if (m_OnEnergyChanged) m_OnEnergyChanged(m_CurrentEnergy);
        return true;
    }

    bool RefillEnergy() {
        m_CurrentEnergy = m_MaxEnergy;
        m_LastRefillTime = std::chrono::system_clock::now();
        if (m_OnEnergyChanged) m_OnEnergyChanged(m_CurrentEnergy);
        return true;
    }

    bool WaitOrPay(int energyCost, std::function<bool(int)> payCallback) {
        Update();
        if (m_CurrentEnergy >= energyCost) {
            ConsumeEnergy(energyCost);
            return true;
        }
        // Offer to pay
        if (payCallback && payCallback(m_RefillCost)) {
            RefillEnergy();
            ConsumeEnergy(energyCost);
            return true;
        }
        return false;
    }

    int GetCurrentEnergy() const { return m_CurrentEnergy; }
    int GetMaxEnergy() const { return m_MaxEnergy; }
    float GetTimeToFullMinutes() const {
        int needed = m_MaxEnergy - m_CurrentEnergy;
        return (float)needed / m_EnergyPerMinute;
    }
    void SetMaxEnergy(int max) { m_MaxEnergy = max; }
    void SetOnEnergyChanged(std::function<void(int)> cb) { m_OnEnergyChanged = cb; }
};

} // namespace NeoEngine
