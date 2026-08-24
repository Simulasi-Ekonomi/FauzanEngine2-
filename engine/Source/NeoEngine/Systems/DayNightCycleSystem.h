#pragma once
#include <string>
#include <cmath>
#include <functional>

namespace NeoEngine {

class DayNightCycleSystem {
private:
    float m_TimeOfDay = 6.0f; // Mulai jam 6 pagi
    float m_DayLength = 1200.0f; // 20 menit realtime = 1 hari game
    float m_SunAngle = 0;
    float m_MoonAngle = 0;
    int m_DayCount = 1;
    bool m_IsDaytime = true;
    std::function<void(bool, int)> m_OnDayNightChange;
    std::function<void(int)> m_OnNewDay;

public:
    void Update(float dt) {
        m_TimeOfDay += 24.0f * dt / m_DayLength;
        if(m_TimeOfDay >= 24.0f) { m_TimeOfDay -= 24.0f; m_DayCount++; if(m_OnNewDay) m_OnNewDay(m_DayCount); }

        bool wasDaytime = m_IsDaytime;
        m_IsDaytime = (m_TimeOfDay >= 6.0f && m_TimeOfDay < 18.0f);
        if(wasDaytime != m_IsDaytime && m_OnDayNightChange) m_OnDayNightChange(m_IsDaytime, m_DayCount);

        m_SunAngle = (m_TimeOfDay - 6.0f) / 12.0f * 180.0f;
        m_MoonAngle = ((m_TimeOfDay + 6.0f) / 12.0f) * 180.0f;
        if(m_SunAngle > 180) m_SunAngle -= 180;
    }

    float GetTimeOfDay() const { return m_TimeOfDay; }
    float GetSunAngle() const { return m_SunAngle; }
    bool IsDaytime() const { return m_IsDaytime; }
    int GetDayCount() const { return m_DayCount; }
    float GetLightIntensity() const { return m_IsDaytime ? 1.0f : 0.3f; }

    void SetDayLength(float seconds) { m_DayLength = seconds; }
    void SetOnDayNightChange(std::function<void(bool, int)> cb) { m_OnDayNightChange = cb; }
    void SetOnNewDay(std::function<void(int)> cb) { m_OnNewDay = cb; }
};

} // namespace NeoEngine
