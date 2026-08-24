#pragma once
#include <vector>
#include <string>
#include <functional>

namespace NeoEngine {

struct Enemy {
    std::string id, type;
    float posX, posY, posZ;
    float speed = 2.0f;
    float health = 100, maxHealth = 100;
    float armor = 0; // % damage reduction
    int goldReward = 10;
    int wave;
    bool airborne = false;
};

struct Turret {
    std::string id, name;
    float posX, posY, posZ;
    float range = 5.0f;
    float damage = 10.0f;
    float fireRate = 1.0f; // shots per second
    float lastFireTime = 0;
    int level = 1;
    int upgradeCost = 50;
    float splashRadius = 0; // 0 = single target
    float slowAmount = 0; // % slow effect
    std::string damageType = "physical"; // physical, fire, ice, poison, energy
};

struct Wave {
    int waveNumber;
    std::vector<Enemy> enemies;
    float spawnInterval = 1.0f;
    float startDelay = 3.0f;
    bool bossWave = false;
};

class TowerDefenseSystem {
private:
    std::vector<Turret> m_Turrets;
    std::vector<Enemy> m_Enemies;
    std::vector<Wave> m_Waves;
    std::vector<std::string> m_Path; // "x,y" waypoints
    int m_CurrentWave = 0;
    float m_WaveTimer = 0;
    int m_Gold = 200;
    int m_Lives = 20;
    int m_Score = 0;
    bool m_WaveActive = false;

public:
    void SetPath(const std::vector<std::string>& path) { m_Path = path; }
    
    Turret& PlaceTurret(const std::string& name, float x, float z) {
        Turret t{"t_" + std::to_string(m_Turrets.size()), name, x, 0, z};
        if (name == "Archer") { t.damage = 15; t.range = 6; t.fireRate = 1.5f; }
        else if (name == "Cannon") { t.damage = 50; t.range = 4; t.fireRate = 0.5f; t.splashRadius = 1.5f; t.upgradeCost = 100; }
        else if (name == "FrostMage") { t.damage = 20; t.range = 5; t.fireRate = 1.2f; t.slowAmount = 40; t.damageType = "ice"; t.upgradeCost = 75; }
        else if (name == "TeslaCoil") { t.damage = 35; t.range = 3.5f; t.fireRate = 2.0f; t.damageType = "energy"; t.upgradeCost = 120; }
        m_Gold -= t.upgradeCost / 2; // initial cost = half of upgrade
        m_Turrets.push_back(t);
        return m_Turrets.back();
    }

    bool UpgradeTurret(int index) {
        if (index < 0 || index >= m_Turrets.size()) return false;
        auto& t = m_Turrets[index];
        if (m_Gold < t.upgradeCost) return false;
        m_Gold -= t.upgradeCost;
        t.level++;
        t.upgradeCost = int(t.upgradeCost * 1.5f);
        t.damage *= 1.3f;
        t.range *= 1.1f;
        t.fireRate *= 1.15f;
        return true;
    }

    void AddWave(const std::string& enemyType, int count, int waveNum, float hp, float spd, int reward) {
        Wave w{waveNum};
        for (int i = 0; i < count; i++) {
            Enemy e{"e_" + std::to_string(waveNum) + "_" + std::to_string(i), enemyType, -10.0f, 0, 0, spd, hp, hp};
            e.goldReward = reward;
            e.wave = waveNum;
            w.enemies.push_back(e);
        }
        m_Waves.push_back(w);
    }

    void Update(float dt) {
        if (!m_WaveActive) {
            m_WaveTimer += dt;
            if (m_WaveTimer >= 5.0f && m_CurrentWave < m_Waves.size()) {
                m_WaveActive = true;
                m_WaveTimer = 0;
            }
            return;
        }

        auto& currentWave = m_Waves[m_CurrentWave];
        m_WaveTimer += dt;

        // Spawn enemies
        if (m_WaveTimer >= currentWave.startDelay) {
            float spawnT = m_WaveTimer - currentWave.startDelay;
            if (spawnT < currentWave.enemies.size() * currentWave.spawnInterval) {
                int idx = int(spawnT / currentWave.spawnInterval);
                if (idx < currentWave.enemies.size()) {
                    auto& e = currentWave.enemies[idx];
                    m_Enemies.push_back(e);
                }
            }
        }

        // Move enemies along path
        for (auto& e : m_Enemies) {
            if (m_Path.empty()) { e.posX += e.speed * dt; continue; }
            // Simple path following: move toward next waypoint
        }

        // Turret targeting
        for (auto& t : m_Turrets) {
            if (m_GameTime - t.lastFireTime < 1.0f / t.fireRate) continue;
            Enemy* target = nullptr;
            float closest = t.range;
            for (auto& e : m_Enemies) {
                float dx = e.posX - t.posX, dz = e.posZ - t.posZ;
                float dist = sqrt(dx*dx + dz*dz);
                if (dist < closest) { target = &e; closest = dist; }
            }
            if (target) {
                float dmg = t.damage;
                if (t.damageType == "fire") dmg *= 1.5f;
                else if (t.damageType == "ice") { dmg *= 0.8f; target->speed *= (1 - t.slowAmount / 100.0f); }
                target->health -= dmg * (1 - target->armor / 100.0f);
                if (t.splashRadius > 0) {
                    for (auto& e : m_Enemies) {
                        if (&e == target) continue;
                        float d = sqrt(pow(e.posX - target->posX, 2) + pow(e.posZ - target->posZ, 2));
                        if (d <= t.splashRadius) e.health -= dmg * 0.5f * (1 - e.armor / 100.0f);
                    }
                }
                t.lastFireTime = m_GameTime;
            }
        }

        // Remove dead enemies & collect gold
        auto it = m_Enemies.begin();
        while (it != m_Enemies.end()) {
            if (it->health <= 0) {
                m_Gold += it->goldReward;
                m_Score += 10 * it->wave;
                it = m_Enemies.erase(it);
            } else {
                ++it;
            }
        }

        // End of waypoint = lose lives
        // (simplified: enemies that reach end disappear and reduce lives)
        auto it2 = m_Enemies.begin();
        while (it2 != m_Enemies.end()) {
            if (it2->posX > 20.0f) { // reached end
                m_Lives--;
                it2 = m_Enemies.erase(it2);
            } else {
                ++it2;
            }
        }

        // Check wave complete
        if (m_Enemies.empty() && m_WaveTimer > currentWave.startDelay + currentWave.enemies.size() * currentWave.spawnInterval) {
            m_WaveActive = false;
            m_CurrentWave++;
            m_WaveTimer = 0;
            m_Gold += 50 + m_CurrentWave * 10;
            if (m_CurrentWave >= m_Waves.size()) {
                // Victory!
                if (m_OnVictory) m_OnVictory();
            }
        }

        if (m_Lives <= 0 && m_OnGameOver) m_OnGameOver();
        m_GameTime += dt;
    }

    int GetGold() const { return m_Gold; }
    int GetLives() const { return m_Lives; }
    int GetScore() const { return m_Score; }
    int GetCurrentWave() const { return m_CurrentWave + 1; }
    const std::vector<Turret>& GetTurrets() const { return m_Turrets; }
    const std::vector<Enemy>& GetEnemies() const { return m_Enemies; }

    void SetOnVictory(std::function<void()> cb) { m_OnVictory = cb; }
    void SetOnGameOver(std::function<void()> cb) { m_OnGameOver = cb; }

private:
    float m_GameTime = 0;
    std::function<void()> m_OnVictory, m_OnGameOver;
};

} // namespace NeoEngine
