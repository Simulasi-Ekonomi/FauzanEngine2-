#pragma once
#include <string>
#include <vector>
#include <functional>

namespace NeoEngine {

struct PlayerInfo {
    int id;
    std::string name;
    float posX, posY, posZ;
    int score = 0;
    int lives = 3;
    bool isAlive = true;
};

enum class GameState { Loading, MainMenu, Playing, Paused, GameOver, Victory };

class GameModeCore {
public:
    GameModeCore() = default;
    
    void SetMaxPlayers(int max) { m_MaxPlayers = max; m_Players.reserve(max); }
    void SetTimeLimit(float seconds) { m_TimeLimit = seconds; }
    void SetScoreToWin(int score) { m_ScoreToWin = score; }
    
    void StartGame() {
        m_State = GameState::Playing;
        m_GameTime = 0;
        m_Players.clear();
        if (m_OnGameStart) m_OnGameStart();
    }
    
    void AddPlayer(int id, const std::string& name) {
        if (m_Players.size() < m_MaxPlayers) {
            m_Players.push_back({id, name, 0, 0, 0, 0, 3, true});
        }
    }
    
    void RemovePlayer(int id) {
        m_Players.erase(std::remove_if(m_Players.begin(), m_Players.end(),
            [id](const PlayerInfo& p) { return p.id == id; }), m_Players.end());
    }
    
    void AddScore(int playerId, int points) {
        for (auto& p : m_Players) {
            if (p.id == playerId) { p.score += points; break; }
        }
        if (m_ScoreToWin > 0) {
            for (auto& p : m_Players) {
                if (p.score >= m_ScoreToWin) {
                    m_State = GameState::Victory;
                    if (m_OnVictory) m_OnVictory(playerId);
                }
            }
        }
    }
    
    void Update(float deltaTime) {
        if (m_State != GameState::Playing) return;
        m_GameTime += deltaTime;
        if (m_TimeLimit > 0 && m_GameTime >= m_TimeLimit) {
            m_State = GameState::GameOver;
            if (m_OnGameOver) m_OnGameOver();
        }
    }
    
    GameState GetState() const { return m_State; }
    float GetGameTime() const { return m_GameTime; }
    const std::vector<PlayerInfo>& GetPlayers() const { return m_Players; }
    
    void SetOnGameStart(std::function<void()> cb) { m_OnGameStart = cb; }
    void SetOnGameOver(std::function<void()> cb) { m_OnGameOver = cb; }
    void SetOnVictory(std::function<void(int)> cb) { m_OnVictory = cb; }

private:
    GameState m_State = GameState::MainMenu;
    std::vector<PlayerInfo> m_Players;
    int m_MaxPlayers = 4, m_ScoreToWin = 0;
    float m_GameTime = 0, m_TimeLimit = 0;
    std::function<void()> m_OnGameStart, m_OnGameOver;
    std::function<void(int)> m_OnVictory;
};

} // namespace NeoEngine
