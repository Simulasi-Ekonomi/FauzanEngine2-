#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <functional>

namespace NeoEngine {

enum class HorrorState { Exploring, Hiding, Chasing, Escaping, Dead };

struct HorrorPlayer {
    std::string id;
    float posX=0, posY=0, posZ=0;
    float sanity=100, maxSanity=100;
    float stealth=0; // 0=tidak bersembunyi, 1=sempurna
    float stamina=100, maxStamina=100;
    HorrorState state=HorrorState::Exploring;
    bool hasFlashlight=true;
    float flashlightBattery=100;
    bool alive=true;
};

struct Monster {
    std::string id, name;
    float posX=0, posY=0, posZ=0;
    float speed=3.0f;
    float detectionRange=20.0f;
    float killRange=2.0f;
    float patrolRadius=50.0f;
    float hearingRange=15.0f;
    int damage=50;
    bool chasing=false;
    std::string targetId;
};

struct Jumpscare {
    std::string id, description;
    float triggerX, triggerY, triggerZ;
    float triggerRadius=3.0f;
    bool triggered=false;
    std::function<void()> onTrigger;
};

class HorrorGameSystem {
private:
    std::vector<HorrorPlayer> m_Players;
    std::vector<Monster> m_Monsters;
    std::vector<Jumpscare> m_Jumpscares;
    float m_DarknessLevel=0.8f;
    std::function<void(const std::string&)> m_OnPlayerDeath;
    std::function<void(const Jumpscare&)> m_OnJumpscare;

public:
    HorrorPlayer* AddPlayer(const std::string& id) {
        m_Players.push_back({id}); return &m_Players.back();
    }
    Monster* AddMonster(const std::string& name, float x, float y, float z) {
        m_Monsters.push_back({"m_"+std::to_string(m_Monsters.size()), name, x, y, z}); return &m_Monsters.back();
    }
    Jumpscare* AddJumpscare(const std::string& desc, float x, float y, float z, float radius=3.0f) {
        m_Jumpscares.push_back({"js_"+std::to_string(m_Jumpscares.size()), desc, x, y, z, radius, false}); return &m_Jumpscares.back();
    }

    void Update(float dt) {
        for(auto& p: m_Players) {
            if(!p.alive) continue;
            // Sanity drain
            if(m_DarknessLevel > 0.5f) p.sanity -= 2.0f * m_DarknessLevel * dt;
            if(p.sanity < 0) p.sanity = 0;
            if(p.flashlightBattery > 0 && p.hasFlashlight) {
                p.flashlightBattery -= 5.0f * dt;
                p.sanity += 3.0f * dt;
                if(p.sanity > p.maxSanity) p.sanity = p.maxSanity;
            }
            // Stamina regen
            p.stamina += 10.0f * dt;
            if(p.stamina > p.maxStamina) p.stamina = p.maxStamina;
        }

        // Monster AI
        for(auto& m: m_Monsters) {
            if(m.chasing && !m.targetId.empty()) {
                HorrorPlayer* target = nullptr;
                for(auto& p: m_Players) if(p.id == m.targetId && p.alive) target = &p;
                if(target) {
                    float dx = target->posX - m.posX, dz = target->posZ - m.posZ;
                    float dist = sqrt(dx*dx + dz*dz);
                    if(dist < m.killRange) {
                        target->alive = false;
                        target->state = HorrorState::Dead;
                        if(m_OnPlayerDeath) m_OnPlayerDeath(target->id);
                        m.chasing = false;
                    } else {
                        m.posX += dx/dist * m.speed * 1.5f * dt;
                        m.posZ += dz/dist * m.speed * 1.5f * dt;
                    }
                }
            } else {
                // Patrol
                for(auto& p: m_Players) {
                    if(!p.alive) continue;
                    float dx = p.posX - m.posX, dz = p.posZ - m.posZ;
                    float dist = sqrt(dx*dx + dz*dz);
                    if(dist < m.detectionRange && p.stealth < 0.5f) {
                        m.chasing = true; m.targetId = p.id; break;
                    }
                }
            }
        }

        // Jumpscare triggers
        for(auto& js: m_Jumpscares) {
            if(js.triggered) continue;
            for(auto& p: m_Players) {
                if(!p.alive) continue;
                float dx = p.posX - js.triggerX, dz = p.posZ - js.triggerZ;
                if(sqrt(dx*dx + dz*dz) < js.triggerRadius) {
                    js.triggered = true;
                    p.sanity -= 30;
                    if(js.onTrigger) js.onTrigger();
                    if(m_OnJumpscare) m_OnJumpscare(js);
                }
            }
        }
    }

    void SetDarkness(float d) { m_DarknessLevel = d; }
    void SetOnPlayerDeath(std::function<void(const std::string&)> cb) { m_OnPlayerDeath = cb; }
    void SetOnJumpscare(std::function<void(const Jumpscare&)> cb) { m_OnJumpscare = cb; }
};

} // namespace NeoEngine
