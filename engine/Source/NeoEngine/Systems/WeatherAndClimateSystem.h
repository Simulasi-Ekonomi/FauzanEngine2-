#pragma once
#include <string>
#include <vector>
#include <cstdlib>
#include <functional>
#include <cmath>

namespace NeoEngine {

enum class Climate { Tropical, Temperate, Arid, Polar, Mediterranean };

struct WeatherEvent {
    std::string type; // "rain", "snow", "fog", "storm", "heatwave", "clear"
    float intensity=1.0f;
    float duration=300.0f;
    float remaining=0;
    bool active=false;
};

class WeatherAndClimateSystem {
private:
    Climate m_Climate = Climate::Temperate;
    float m_Temperature = 25.0f;
    float m_Humidity = 0.5f;
    float m_WindSpeed = 5.0f;
    WeatherEvent m_CurrentWeather;
    std::vector<WeatherEvent> m_Forecast;
    float m_Season = 0; // 0-3: spring,summer,autumn,winter
    float m_SeasonProgress = 0;
    std::function<void(const WeatherEvent&)> m_OnWeatherChange;

public:
    void Update(float dt, float dayLength=1200.0f) {
        m_SeasonProgress += dt;
        if(m_SeasonProgress >= dayLength * 90.0f) {
            m_SeasonProgress = 0;
            m_Season = fmod(m_Season + 1, 4.0f);
            UpdateSeasonalEffects();
        }

        if(m_CurrentWeather.remaining > 0) {
            m_CurrentWeather.remaining -= dt;
        } else {
            GenerateRandomWeather();
        }

        m_Temperature += (rand()%100 - 50) / 100.0f * dt / 60.0f;
    }

    void UpdateSeasonalEffects() {
        switch((int)m_Season) {
            case 0: m_Temperature = 15.0f; break; // Spring
            case 1: m_Temperature = 30.0f; break; // Summer
            case 2: m_Temperature = 20.0f; break; // Autumn
            case 3: m_Temperature = 5.0f; break;  // Winter
        }
    }

    void GenerateRandomWeather() {
        int r = rand() % 100;
        if(r < 50) m_CurrentWeather = {"clear", 0, 600.0f, 600.0f, true};
        else if(r < 75) m_CurrentWeather = {"rain", 0.5f, 300.0f, 300.0f, true};
        else if(r < 85) m_CurrentWeather = {"fog", 0.8f, 200.0f, 200.0f, true};
        else if(r < 95) m_CurrentWeather = {"storm", 1.0f, 150.0f, 150.0f, true};
        else m_CurrentWeather = {"snow", 0.3f, 400.0f, 400.0f, true};

        if(m_OnWeatherChange) m_OnWeatherChange(m_CurrentWeather);
    }

    float GetTemperature() const { return m_Temperature; }
    float GetSeason() const { return m_Season; }
    const WeatherEvent& GetCurrentWeather() const { return m_CurrentWeather; }
    void SetClimate(Climate c) { m_Climate = c; }
    void SetOnWeatherChange(std::function<void(const WeatherEvent&)> cb) { m_OnWeatherChange = cb; }
};

} // namespace NeoEngine
