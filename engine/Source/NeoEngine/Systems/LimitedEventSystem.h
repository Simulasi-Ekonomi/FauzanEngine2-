#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <unordered_map>

namespace NeoEngine {

enum class EventType { DoubleXP, DoubleGold, SpecialBoss, TreasureHunt, LoginEvent, CollectionEvent, BattlePass, LimitedShop };

struct LimitedEvent {
    std::string id;
    std::string name;
    std::string description;
    EventType type;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    std::unordered_map<std::string, float> modifiers;
    bool active = true;
};

class LimitedEventSystem {
private:
    std::vector<LimitedEvent> m_Events;
    std::function<void(const LimitedEvent&)> m_OnEventStart;
    std::function<void(const LimitedEvent&)> m_OnEventEnd;

public:
    void CreateEvent(const std::string& name, const std::string& desc, EventType type, int durationHours) {
        LimitedEvent e{name, name, desc, type, std::chrono::system_clock::now(), 
                       std::chrono::system_clock::now() + std::chrono::hours(durationHours)};
        m_Events.push_back(e);
        if (m_OnEventStart) m_OnEventStart(e);
    }

    float GetModifier(const std::string& key) const {
        for (auto& e : m_Events) {
            if (!e.active) continue;
            auto now = std::chrono::system_clock::now();
            if (now < e.startTime || now > e.endTime) continue;
            auto it = e.modifiers.find(key);
            if (it != e.modifiers.end()) return it->second;
        }
        return 1.0f;
    }

    void Update() {
        auto now = std::chrono::system_clock::now();
        for (auto& e : m_Events) {
            if (e.active && now > e.endTime) {
                e.active = false;
                if (m_OnEventEnd) m_OnEventEnd(e);
            }
        }
    }

    std::vector<LimitedEvent> GetActiveEvents() const {
        std::vector<LimitedEvent> active;
        auto now = std::chrono::system_clock::now();
        for (auto& e : m_Events) {
            if (e.active && now >= e.startTime && now <= e.endTime) active.push_back(e);
        }
        return active;
    }

    void SetOnEventStart(std::function<void(const LimitedEvent&)> cb) { m_OnEventStart = cb; }
    void SetOnEventEnd(std::function<void(const LimitedEvent&)> cb) { m_OnEventEnd = cb; }
};

} // namespace NeoEngine
