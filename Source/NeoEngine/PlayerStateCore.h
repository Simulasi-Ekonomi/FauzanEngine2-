#pragma once
#include <string>
#include <vector>

namespace NeoEngine {

struct PlayerState {
    std::string playerName;
    int score = 0;
    int level = 1;
    float health = 100;
    float maxHealth = 100;
    std::vector<std::string> inventory;
};

class PlayerStateCore {
public:
    void SetPlayerName(const std::string& name) { m_State.playerName = name; }
    std::string GetPlayerName() const { return m_State.playerName; }
    int GetScore() const { return m_State.score; }
    void AddScore(int points) { m_State.score += points; }
    int GetLevel() const { return m_State.level; }
    void LevelUp() { m_State.level++; }
    float GetHealth() const { return m_State.health; }
    void TakeDamage(float dmg) { m_State.health -= dmg; }
    bool IsAlive() const { return m_State.health > 0; }
    void AddItem(const std::string& item) { m_State.inventory.push_back(item); }

private:
    PlayerState m_State;
};

} // namespace NeoEngine
