#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <functional>

namespace NeoEngine {
struct Racer { std::string id, name; float posX=0, posZ=0; float speed=0, maxSpeed=80; float acceleration=15, handling=3; float lapTime=0; int lap=0; bool finished=false; float totalTime=0; };
struct RaceTrack { std::string name; std::vector<std::pair<float,float>> checkpoints; int totalLaps=3; float trackLength=1000; };
class RacingSystem {
private:
    std::vector<Racer> m_Racers; RaceTrack m_Track; bool m_RaceActive=false; float m_RaceTime=0;
    std::function<void(const Racer&)> m_OnFinish; std::function<void(const std::vector<Racer>&)> m_OnRaceEnd;
public:
    void SetTrack(const RaceTrack& t) { m_Track = t; }
    void AddRacer(const std::string& id, const std::string& name, float maxSpeed, float accel, float handling) {
        m_Racers.push_back({id, name, 0, 0, 0, maxSpeed, accel, handling}); }
    void StartRace() { m_RaceActive = true; m_RaceTime = 0; for (auto& r : m_Racers) { r.posX = 0; r.posZ = 0; r.speed = 0; r.lap = 0; r.lapTime = 0; r.totalTime = 0; r.finished = false; } }
    void UpdateRace(float dt) {
        if (!m_RaceActive) return; m_RaceTime += dt;
        for (auto& r : m_Racers) {
            if (r.finished) continue;
            r.speed += r.acceleration * dt; if (r.speed > r.maxSpeed) r.speed = r.maxSpeed;
            r.posX += r.speed * dt; r.lapTime += dt; r.totalTime += dt;
            if (r.posX >= m_Track.trackLength * r.lap) { r.lap++;
                if (r.lap >= m_Track.totalLaps) { r.finished = true; if (m_OnFinish) m_OnFinish(r); }
            }
        }
        bool allDone = true; for (auto& r : m_Racers) if (!r.finished) { allDone = false; break; }
        if (allDone) { m_RaceActive = false; if (m_OnRaceEnd) m_OnRaceEnd(m_Racers); }
    }
    bool IsRaceActive() const { return m_RaceActive; }
    const auto& GetRacers() const { return m_Racers; }
    void SetOnFinish(std::function<void(const Racer&)> cb) { m_OnFinish = cb; }
    void SetOnRaceEnd(std::function<void(const std::vector<Racer>&)> cb) { m_OnRaceEnd = cb; }
};
}
