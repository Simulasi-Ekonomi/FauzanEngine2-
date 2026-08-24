#pragma once
#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include <unordered_map>

namespace NeoEngine {

struct ScheduledEvent {
    std::string id, name, description;
    std::chrono::system_clock::time_point startTime, endTime;
    bool recurring = false;
    int recurringHours = 24;
    bool active = false;
    std::function<void()> onStart;
    std::function<void()> onEnd;
    std::unordered_map<std::string, float> modifiers;
};

class GameEventScheduler {
private:
    std::vector<ScheduledEvent> m_Events;
    
public:
    ScheduledEvent* ScheduleEvent(const std::string& name, int startDelayMinutes, int durationMinutes) {
        auto now = std::chrono::system_clock::now();
        m_Events.push_back({name, name, "", now + std::chrono::minutes(startDelayMinutes),
                           now + std::chrono::minutes(startDelayMinutes + durationMinutes), false});
        return &m_Events.back();
    }

    ScheduledEvent* ScheduleRecurringEvent(const std::string& name, int intervalHours, int durationMinutes) {
        auto now = std::chrono::system_clock::now();
        m_Events.push_back({name, name, "", now, now + std::chrono::minutes(durationMinutes),
                           true, intervalHours, false});
        return &m_Events.back();
    }

    void Update() {
        auto now = std::chrono::system_clock::now();
        for (auto& e : m_Events) {
            if (!e.active && now >= e.startTime && now <= e.endTime) {
                e.active = true;
                if (e.onStart) e.onStart();
            }
            if (e.active && now > e.endTime) {
                e.active = false;
                if (e.onEnd) e.onEnd();
                if (e.recurring) {
                    e.startTime = e.startTime + std::chrono::hours(e.recurringHours);
                    e.endTime = e.endTime + std::chrono::hours(e.recurringHours);
                }
            }
        }
    }

    std::vector<ScheduledEvent*> GetActiveEvents() {
        std::vector<ScheduledEvent*> active;
        for (auto& e : m_Events) if (e.active) active.push_back(&e);
        return active;
    }
};

} // namespace NeoEngine
