#pragma once
#include <vector>
#include <string>
#include <cmath>

namespace NeoEngine {
struct StealthActor { std::string id; float visibility=1.0f; float noiseLevel=0; float suspicion=0; bool detected=false; float lastKnownX=0, lastKnownY=0, lastKnownZ=0; };
struct Guard { std::string id; float posX, posY, posZ; float viewAngle=90, viewRange=15; float alertLevel=0; };
class StealthSystem {
private:
    std::vector<StealthActor> m_Actors; std::vector<Guard> m_Guards;
public:
    StealthActor* CreateActor(const std::string& id) { m_Actors.push_back({id}); return &m_Actors.back(); }
    Guard* CreateGuard(float x, float y, float z) { m_Guards.push_back({"g_"+std::to_string(m_Guards.size()), x, y, z}); return &m_Guards.back(); }
    void UpdateVisibility(StealthActor& a, float lightLevel, float coverFactor) { a.visibility = std::min(1.0f, std::max(0.0f, lightLevel * (1.0f - coverFactor))); }
    bool CheckDetection(StealthActor& a, float posX, float posY, float posZ) {
        for (auto& g : m_Guards) { float dx = posX - g.posX, dy = posY - g.posY, dz = posZ - g.posZ; float dist = sqrt(dx*dx+dy*dy+dz*dz);
            if (dist < g.viewRange && a.visibility > 0.3f) { a.suspicion += 0.1f; a.lastKnownX = posX; a.lastKnownY = posY; a.lastKnownZ = posZ; if (a.suspicion > 1.0f) { a.detected = true; return true; } } }
        return false;
    }
    void ResetDetection(StealthActor& a) { a.suspicion *= 0.9f; a.detected = false; }
    float GetSuspicion(const std::string& actorId) const { for (auto& a : m_Actors) if (a.id == actorId) return a.suspicion; return 0; }
};
}
